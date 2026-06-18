#pip install customtkinter   # para la UI
#pip install matplotlib  

"""
Robot Control - Monitor de Servos
==================================
Formato UART esperado del LPC1769:
  $S1:<angulo1>,S2:<angulo2>,M:<modo>\n
  Ejemplo: $S1:90,S2:45,M:MANUAL\n
 
La PC puede enviar comandos:
  AUTO\n   -> activa modo automático
  MANUAL\n -> activa modo manual
"""
 
import tkinter as tk
from tkinter import ttk
import threading
import serial
import serial.tools.list_ports
import math
import time
from collections import deque
 
# ── Constantes de diseño ────────────────────────────────────────────────────
BG        = "#0f1117"
PANEL     = "#1a1d27"
ACCENT    = "#4f8ef7"
ACCENT2   = "#f74f7a"
GREEN     = "#4fbd6e"
YELLOW    = "#f7c44f"
TEXT      = "#e8eaf0"
SUBTEXT   = "#6b7280"
RADIUS    = 12
 
# ── Datos simulados para demo sin hardware ───────────────────────────────────
DEMO_MODE = False   # cambiar a False cuando tengas el LPC conectado
 
class ServoGauge(tk.Canvas):
    """Gauge semicircular para mostrar ángulo de un servo."""
    def __init__(self, parent, label, color, **kwargs):
        super().__init__(parent, bg=PANEL, highlightthickness=0, **kwargs)
        self.label  = label
        self.color  = color
        self.angle  = 0
        self._draw(0)
 
    def _draw(self, angle):
        self.delete("all")
        w = int(self["width"])
        h = int(self["height"])
        cx, cy = w // 2, int(h * 0.62)
        r_outer = int(min(w, h) * 0.42)
        r_inner = int(r_outer * 0.68)
 
        # Arco de fondo (gris)
        self._arc(cx, cy, r_outer, r_inner, 180, 180, "#2a2d3a")
 
        # Arco coloreado según ángulo (0-360 mapeado a 0-180 grados de arco)
        extent = (angle / 360) * 180
        if extent > 0:
            self._arc(cx, cy, r_outer, r_inner, 180, extent, self.color)
 
        # Aguja
        rad = math.radians(180 - (angle / 360) * 180)
        nx  = cx + int(r_inner * 0.82 * math.cos(rad))
        ny  = cy - int(r_inner * 0.82 * math.sin(rad))
        self.create_line(cx, cy, nx, ny, fill=TEXT, width=3, capstyle="round")
        self.create_oval(cx-6, cy-6, cx+6, cy+6, fill=self.color, outline="")
 
        # Texto ángulo
        self.create_text(cx, cy + int(h * 0.18),
                         text=f"{angle:.0f}°",
                         fill=TEXT, font=("Arial", 22, "bold"))
        # Label
        self.create_text(cx, int(h * 0.10),
                         text=self.label,
                         fill=SUBTEXT, font=("Arial", 11))
        # Marcas 0 y 360
        for deg, label in [(0, "0°"), (180, "360°"), (90, "180°")]:
            rad2 = math.radians(180 - deg)
            mx = cx + int((r_outer + 12) * math.cos(rad2))
            my = cy - int((r_outer + 12) * math.sin(rad2))
            self.create_text(mx, my, text=label,
                             fill=SUBTEXT, font=("Arial", 8))
 
    def _arc(self, cx, cy, r_out, r_in, start, extent, color):
        """Dibuja un arco grueso como anillo."""
        steps = max(int(extent * 2), 2)
        for i in range(steps):
            a0 = math.radians(start - (extent * i / steps))
            a1 = math.radians(start - (extent * (i+1) / steps))
            for r in range(r_in, r_out, 2):
                x0 = cx + r * math.cos(a0)
                y0 = cy - r * math.sin(a0)
                x1 = cx + r * math.cos(a1)
                y1 = cy - r * math.sin(a1)
                self.create_line(x0, y0, x1, y1,
                                 fill=color, width=3, capstyle="round")
 
    def set_angle(self, angle):
        self.angle = max(0, min(360, angle))
        self._draw(self.angle)
 
 
class RobotControlApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Robot Control")
        self.root.configure(bg=BG)
        self.root.geometry("720x580")
        self.root.resizable(False, False)
 
        self.serial_port  = None
        self.running      = False
        self.mode         = tk.StringVar(value="MANUAL")
        self.status_msg   = tk.StringVar(value="Desconectado")
        self.log_lines    = deque(maxlen=50)
        self._demo_t      = 0
 
        self._build_ui()
 
        if DEMO_MODE:
            self._start_demo()
            
    """"    self.root.after(1000, self.prueba_gauges)
        
    def prueba_gauges(self):
        self.gauge1.set_angle(90)
        self.gauge2.set_angle(270)

        print("GAUGES OK")"""
 
    # ── UI ───────────────────────────────────────────────────────────────────
    def _build_ui(self):
        # ── Título ──
        title_frame = tk.Frame(self.root, bg=BG)
        title_frame.pack(fill="x", padx=24, pady=(18, 0))
 
        tk.Label(title_frame, text="⚙  Robot Control",
                 bg=BG, fg=TEXT, font=("Arial", 18, "bold")).pack(side="left")
 
        # Badge de modo
        self.mode_badge = tk.Label(title_frame, textvariable=self.mode,
                                   bg=GREEN, fg="#fff",
                                   font=("Arial", 10, "bold"),
                                   padx=10, pady=3)
        self.mode_badge.pack(side="right", padx=4)
 
        # ── Conexión ──
        conn_frame = tk.Frame(self.root, bg=PANEL)
        conn_frame.pack(fill="x", padx=24, pady=(12, 0))
        conn_frame.configure(pady=8)
 
        tk.Label(conn_frame, text="Puerto:", bg=PANEL, fg=SUBTEXT,
                 font=("Arial", 10)).pack(side="left", padx=(12, 4))
 
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(conn_frame, textvariable=self.port_var,
                                        width=12, state="readonly")
        self.port_combo.pack(side="left", padx=4)
        self._refresh_ports()
 
        tk.Label(conn_frame, text="Baud:", bg=PANEL, fg=SUBTEXT,
                 font=("Arial", 10)).pack(side="left", padx=(10, 4))
        self.baud_var = tk.StringVar(value="9600")
        ttk.Combobox(conn_frame, textvariable=self.baud_var,
                     values=["9600","19200","38400","57600","115200"],
                     width=8, state="readonly").pack(side="left", padx=4)
 
        self.connect_btn = tk.Button(conn_frame, text="Conectar",
                                     bg=ACCENT, fg="#fff",
                                     font=("Arial", 10, "bold"),
                                     relief="flat", padx=14, pady=4,
                                     cursor="hand2",
                                     command=self._toggle_connect)
        self.connect_btn.pack(side="left", padx=(12, 4))
 
        tk.Button(conn_frame, text="↺", bg=PANEL, fg=SUBTEXT,
                  relief="flat", font=("Arial", 12),
                  cursor="hand2",
                  command=self._refresh_ports).pack(side="left")
 
        tk.Label(conn_frame, textvariable=self.status_msg,
                 bg=PANEL, fg=SUBTEXT,
                 font=("Arial", 9)).pack(side="right", padx=12)
 
        # ── Gauges ──
        gauge_frame = tk.Frame(self.root, bg=BG)
        gauge_frame.pack(fill="x", padx=24, pady=(16, 0))
 
        self.gauge1 = ServoGauge(gauge_frame, "Servo 1", ACCENT,
                                  width=300, height=200)
        self.gauge1.pack(side="left", padx=(0, 12))
 
        self.gauge2 = ServoGauge(gauge_frame, "Servo 2", ACCENT2,
                                  width=300, height=200)
        self.gauge2.pack(side="left")
 
        # ── Botones de modo ──
        btn_frame = tk.Frame(self.root, bg=BG)
        btn_frame.pack(fill="x", padx=24, pady=(18, 0))
 
        self.auto_btn = tk.Button(
            btn_frame,
            text="▶  Modo Automático\nEjecuta posiciones guardadas",
            bg=GREEN, fg="#fff",
            font=("Arial", 12, "bold"),
            relief="flat", padx=20, pady=14,
            cursor="hand2", justify="center",
            command=self._set_auto)
        self.auto_btn.pack(side="left", expand=True, fill="x", padx=(0, 8))
 
        self.manual_btn = tk.Button(
            btn_frame,
            text="🎮  Modo Manual\nControl con potenciómetro",
            bg=ACCENT, fg="#fff",
            font=("Arial", 12, "bold"),
            relief="flat", padx=20, pady=14,
            cursor="hand2", justify="center",
            command=self._set_manual)
        self.manual_btn.pack(side="left", expand=True, fill="x", padx=(8, 0))
 
        # ── Log ──
        log_frame = tk.Frame(self.root, bg=PANEL)
        log_frame.pack(fill="both", expand=True, padx=24, pady=(14, 18))
 
        tk.Label(log_frame, text="Log UART", bg=PANEL, fg=SUBTEXT,
                 font=("Arial", 9, "bold")).pack(anchor="w", padx=10, pady=(6,0))
 
        self.log_text = tk.Text(log_frame, bg=PANEL, fg=SUBTEXT,
                                 font=("Consolas", 9),
                                 relief="flat", state="disabled",
                                 height=5)
        self.log_text.pack(fill="both", expand=True, padx=10, pady=(2, 8))
 
    # ── Conexión serial ──────────────────────────────────────────────────────
    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo["values"] = ports
        if ports:
            self.port_var.set(ports[0])
 
    def _toggle_connect(self):
        if self.serial_port and self.serial_port.is_open:
            self._disconnect()
        else:
            self._connect()
 
    def _connect(self):
        try:
            self.serial_port = serial.Serial(
                self.port_var.get(),
                int(self.baud_var.get()),
                timeout=1
            )
            self.running = True
            self.status_msg.set(f"Conectado a {self.port_var.get()}")
            self.connect_btn.config(text="Desconectar", bg=ACCENT2)
            threading.Thread(target=self._read_loop, daemon=True).start()
            self._log(f"Conectado a {self.port_var.get()} @ {self.baud_var.get()}")
        except Exception as e:
            self.status_msg.set(f"Error: {e}")
            self._log(f"ERROR: {e}")
 
    def _disconnect(self):
        self.running = False
        if self.serial_port:
            self.serial_port.close()
        self.status_msg.set("Desconectado")
        self.connect_btn.config(text="Conectar", bg=ACCENT)
        self._log("Desconectado")
 
    # ── Lectura UART ─────────────────────────────────────────────────────────
    def _read_loop(self):
        while self.running and self.serial_port.is_open:
            try:
                raw = self.serial_port.readline()

                if not raw:
                    continue

                line = raw.decode(
                    "utf-8",
                    errors="ignore"
                ).strip()

                print("RX:", repr(line))

                self.root.after(0, self._parse_line, line)

            except Exception as e:
                print("READ ERROR:", e)
                break
    #def _read_loop(self):
     #   """Lee líneas del puerto serie en un hilo separado."""
      #  while self.running and self.serial_port.is_open:
       #     try:
        #        line = self.serial_port.readline().decode("utf-8", errors="ignore").strip()
         #       if line.startswith("$"):
          #          self.root.after(0, self._parse_line, line)
           # except:
            #    break
 
    def _parse_line(self, line):

        self._log(line)

        try:

            if not line.startswith("$"):
                print("Formato inválido:", repr(line))
                return

            data = {}

            partes = line[1:].split(",")

            for parte in partes:

                if ":" not in parte:
                    continue

                k, v = parte.split(":", 1)

                data[k.strip()] = v.strip()

            print("DATA:", data)

            if "S1" in data:
                ang1 = float(data["S1"])
                self.gauge1.set_angle(ang1)

            if "S2" in data:
                ang2 = float(data["S2"])
                self.gauge2.set_angle(ang2)

            if "M" in data:
                self._update_mode_display(data["M"])

        except Exception as e:
            print("PARSE ERROR:", e)
            self._log(f"Parse error: {e}")
        
        
    """def _parse_line(self, line):
        
        #Parsea: $S1:90,S2:45,M:MANUAL
        
        try:
            self._log(line)
            parts = line[1:].split(",")
            data = {}
            for p in parts:
                k, v = p.split(":")
                data[k.strip()] = v.strip()
 
            if "S1" in data:
                self.gauge1.set_angle(float(data["S1"]))
            if "S2" in data:
                self.gauge2.set_angle(float(data["S2"]))
            if "M" in data:
                self._update_mode_display(data["M"])
        except Exception as e:
            self._log(f"Parse error: {e}")
    """
    # ── Comandos ─────────────────────────────────────────────────────────────
    def _set_auto(self):
        self._send_command("0")
        self._update_mode_display("AUTO")
 
    def _set_manual(self):
        self._send_command("1")
        self._update_mode_display("MANUAL")
 
    def _send_command(self, cmd):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.write(f"{cmd}\n".encode())
            self._log(f"→ Enviado: {cmd}")
        elif DEMO_MODE:
            self._log(f"→ [DEMO] Comando: {cmd}")
 
    def _update_mode_display(self, mode):
        self.mode.set(mode)
        if mode == "AUTO":
            self.mode_badge.config(bg=GREEN)
        else:
            self.mode_badge.config(bg=ACCENT)
 
    # ── Log ──────────────────────────────────────────────────────────────────
    def _log(self, msg):
        ts = time.strftime("%H:%M:%S")
        line = f"[{ts}] {msg}\n"
        self.log_text.config(state="normal")
        self.log_text.insert("end", line)
        self.log_text.see("end")
        self.log_text.config(state="disabled")
 
    # ── Demo ─────────────────────────────────────────────────────────────────
    def _start_demo(self):
        self.status_msg.set("Modo demo (sin hardware)")
        self._log("Modo DEMO activo — conectá el LPC1769 y cambiá DEMO_MODE=False")
        self._demo_tick()
 
    def _demo_tick(self):
        self._demo_t += 0.05
        a1 = (math.sin(self._demo_t) * 0.5 + 0.5) * 360
        a2 = (math.cos(self._demo_t * 0.7) * 0.5 + 0.5) * 360
        self.gauge1.set_angle(a1)
        self.gauge2.set_angle(a2)
        self.root.after(50, self._demo_tick)
 
 
if __name__ == "__main__":
    root = tk.Tk()
    app  = RobotControlApp(root)
    root.mainloop()
