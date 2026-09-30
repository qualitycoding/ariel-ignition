"""Generates wiring-tci.svg and wiring-magbreak.svg (block wiring diagrams).
Authoritative connections are the netlists in WIRING.md; run: python3 draw_wiring.py"""
from html import escape as esc
W, H = 1000, 700
def svg(body, title):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" font-family="Helvetica,Arial,sans-serif" font-size="13">
<rect width="{W}" height="{H}" fill="#ffffff"/>
<text x="20" y="32" font-size="20" font-weight="bold">{title}</text>
<defs><marker id="dot" viewBox="0 0 10 10" refX="5" refY="5" markerWidth="6" markerHeight="6"><circle cx="5" cy="5" r="4" fill="#222"/></marker></defs>
{body}
<text x="20" y="{H-14}" font-size="11" fill="#555">Block wiring only — the netlist in hardware/WIRING.md is authoritative. Ariel 350 electronic ignition, qualitycoding/ariel-ignition.</text>
</svg>'''
def box(x, y, w, h, title, lines=(), fill="#eef4fb", stroke="#2a5d9f"):
    s = f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="{fill}" stroke="{stroke}" stroke-width="2"/>'
    s += f'<text x="{x+10}" y="{y+20}" font-weight="bold">{esc(title)}</text>'
    for i, l in enumerate(lines):
        s += f'<text x="{x+10}" y="{y+40+i*17}" font-size="12">{esc(l)}</text>'
    return s
def wire(pts, color="#222", label=None, lx=None, ly=None, width=2, dash=None):
    d = " ".join(f"{x},{y}" for x, y in pts)
    s = f'<polyline points="{d}" fill="none" stroke="{color}" stroke-width="{width}"' + (f' stroke-dasharray="{dash}"' if dash else "") + '/>'
    if label: s += f'<text x="{lx}" y="{ly}" font-size="11" fill="{color}">{esc(label)}</text>'
    return s
def gnd(x, y):
    return (f'<line x1="{x}" y1="{y}" x2="{x}" y2="{y+10}" stroke="#222" stroke-width="2"/>'
            f'<line x1="{x-12}" y1="{y+10}" x2="{x+12}" y2="{y+10}" stroke="#222" stroke-width="2"/>'
            f'<line x1="{x-7}" y1="{y+15}" x2="{x+7}" y2="{y+15}" stroke="#222" stroke-width="2"/>'
            f'<line x1="{x-3}" y1="{y+20}" x2="{x+3}" y2="{y+20}" stroke="#222" stroke-width="2"/>')

def controller(x, y, variant):
    lines = ["Arduino Pro Mini 5V/16MHz (ATmega328P @ 8 MHz)",
             "D8/PB0 ICP1 ← TRIG (4k7 pull-up, 1k + 1nF)",
             "D9/PB1 OC1A → gate network 220R / 470k / 10nF",
             "D2/PD2 ← KILL (10k pull-up, 1k + 100nF)",
             "D3/PD3 ← MAP switch (optional)",
             "A6 ← supply sense 68k / 4k7 (1.1 V ref)",
             "D0/D1 = tuning lead 38400 8N1; ISP 6-pin"]
    if variant == "TCI":
        lines.append("U1 LM2936-5.0 + SS14 + 10R + SMBJ20A + 100µF")
    else:
        lines.append("D10/PB2 → harvest isolate (HF-B only)")
        lines.append("Supply: harvest front end HF-A / HF-B (gate G-003)")
    return box(x, y, 380, 40 + 17 * len(lines) + 8, "Controller (alloy box, ~90×40×30 mm)", lines)

def trigger(x, y):
    return box(x, y, 280, 132, "Trigger (in magneto breaker housing)",
               ["Acetal disc on spindle (replaces points)",
                "SmCo magnet S-out @ lead 50° BTDC",
                "SmCo magnet N-out @ trail 0° (TDC)",
                "A1220LUA-T Hall latch, gap 1.0–1.5 mm",
                "(25° apart on the half-speed spindle)"], fill="#f3f0fa", stroke="#6a4c9c")

def tci():
    b = ""
    b += box(30, 60, 190, 90, "Battery 6 V (or 12 V)", ["negative earth", "charged by dynamo/reg."], "#fdf3e7", "#b86b00")
    b += box(270, 60, 150, 70, "SW1 main switch", ["hard stop (power off)"], "#fdf3e7", "#b86b00")
    b += box(460, 60, 110, 70, "F1 5 A", ["blade fuse"], "#fdf3e7", "#b86b00")
    b += box(640, 60, 300, 110, "L1 ignition coil", ["6 V: primary 1.2–1.8 Ω", "12 V: primary 2.8–3.5 Ω", "(+) ← IGN+    (−) → Q1 collector"], "#fbeaea", "#a33")
    b += box(700, 230, 240, 110, "Q1 ignition IGBT", ["FGD3040G2-F085V / ISL9V3040", "400 V internal clamp, logic gate", "emitter → PGND star → frame"], "#fbeaea", "#a33")
    b += box(700, 400, 240, 80, "Spark plug", ["copper HT lead + 5 kΩ cap"], "#fbeaea", "#a33")
    b += controller(250, 230, "TCI")
    b += trigger(30, 470)
    b += box(360, 520, 300, 100, "Rider controls", ["SW2 handlebar STOP button (N/O) → KILL", "SW3 map toggle (optional) → MAP", "Tuning lead (FTDI 5 V) → J2"], "#eef8ee", "#2e7d32")
    b += wire([(220, 95), (270, 95)], "#b86b00") + wire([(420, 95), (460, 95)], "#b86b00")
    b += wire([(570, 95), (640, 95)], "#b86b00", "IGN+", 590, 88)
    b += wire([(600, 95), (600, 200), (440, 200), (440, 230)], "#b86b00", "IGN+ → supply & sense", 452, 196)
    b += wire([(805, 170), (805, 230)], "#a33", "coil −", 812, 205)
    b += wire([(630, 300), (700, 300)], "#2a5d9f", "GATE", 640, 293)
    b += wire([(940, 120), (975, 120), (975, 440), (940, 440)], "#a33", "HT", 979, 300)
    b += wire([(820, 340), (820, 365)], "#222") + gnd(820, 365)
    b += wire([(310, 480), (310, 420)], "#6a4c9c", "TRIG / +5V / GND (screened)", 318, 460)
    b += wire([(500, 520), (500, 420)], "#2e7d32", "KILL / MAP", 508, 470)
    b += wire([(60, 150), (60, 200)], "#222") + gnd(60, 200)
    return svg(b, "Ariel 350 electronic ignition — TCI (battery) variant")

def mag():
    b = ""
    b += box(30, 60, 400, 150, "Retained Lucas magneto (points removed)",
             ["armature: primary + secondary, condenser, slip ring", "HT pick-up and lead to the plug (original)",
              "cut-out terminal = primary live end P", "still drives the dynamo (magdyno)"], "#fdf3e7", "#b86b00")
    b += box(560, 60, 400, 110, "Q2 electronic breaker (ignition IGBT)",
             ["replaces the points: closed = primary shorted", "opens at the spark angle: magneto fires",
              "re-closes 60 deg later (magopen)"], "#fbeaea", "#a33")
    b += box(560, 230, 400, 110, "Harvest front end (chosen at gate G-003)",
             ["HF-A: rectify the unused flux reversal", "HF-B: bridge + depletion-MOSFET isolator (PB2)",
              "reservoir -> 5 V LDO -> controller"], "#fff8e1", "#b8860b")
    b += box(30, 240, 200, 60, "Spark plug", ["original HT lead + 5 kOhm cap"], "#fbeaea", "#a33")
    b += controller(30, 330, "MAG")
    b += trigger(560, 400)
    b += box(560, 560, 400, 80, "Rider controls", ["SW2 cut-out button: shorts P to earth (hard kill)", "also sensed on PD2: firmware holds Q2 closed"], "#eef8ee", "#2e7d32")
    b += wire([(430, 110), (560, 110)], "#a33", "P (cut-out terminal)", 440, 102)
    b += wire([(495, 110), (495, 285), (560, 285)], "#a33")
    b += wire([(410, 380), (520, 380), (520, 150), (560, 150)], "#2a5d9f", "GATE", 440, 373)
    b += wire([(560, 320), (540, 320), (540, 430), (410, 430)], "#b8860b", "+5 V", 440, 423)
    b += wire([(100, 210), (100, 240)], "#a33", "HT", 106, 230, dash="6,4")
    b += wire([(560, 470), (410, 470)], "#6a4c9c", "TRIG (screened)", 430, 463)
    b += wire([(560, 610), (300, 610), (300, 531)], "#2e7d32", "KILL sense", 310, 603)
    b += wire([(960, 600), (985, 600), (985, 40), (495, 40), (495, 110)], "#2e7d32", "SW2 to P", 900, 34, dash="6,4")
    return svg(b, "Ariel 350 electronic ignition - MAGBREAK (battery-less) variant")

if __name__ == "__main__":
    open("wiring-tci.svg", "w").write(tci())
    open("wiring-magbreak.svg", "w").write(mag())
