"""Reference (exact-integer) implementations used to derive the expected
values frozen in tests/unit/test_timing.c and tests/sim/ scenarios.
Spike only: never imported by firmware. Run: python3 ref.py"""
import math

def rnd(n, d):                       # round half away from zero
    q, r = divmod(abs(n), d); q += (2 * r >= d); return q if n >= 0 else -q
def rpm_from_period_us(p, d): return 0 if p == 0 else min(65535, 60_000_000 * d // p)
def angle_to_us(cdeg, p, d): return rnd(cdeg * p, 36000 * d)
def curve(rpm, R, A):
    if rpm <= R[0]: return A[0]
    if rpm >= R[-1]: return A[-1]
    for i in range(len(R) - 1):
        if R[i] <= rpm < R[i + 1]:
            return A[i] + int((A[i + 1] - A[i]) * (rpm - R[i]) / (R[i + 1] - R[i]))
def segment_delay_us(s, l, t, a, lat):
    if l <= t or a >= l: return -1
    x = rnd(s * (l - a), l - t) - lat; return -1 if x < 0 else x
def piston_travel_to_deg(x_mm, stroke_mm=85.0, rod_mm=178.0):
    """crank angle BTDC (deg) for piston x_mm below TDC (bisection)."""
    r = stroke_mm / 2; lo, hi = 0.0, 90.0
    for _ in range(80):
        m = (lo + hi) / 2; t = math.radians(m)
        p = r * (1 - math.cos(t)) + rod_mm - math.sqrt(rod_mm**2 - (r * math.sin(t))**2)
        lo, hi = (m, hi) if p < x_mm else (lo, m)
    return m

if __name__ == "__main__":
    R = [500,800,1200,1800,2400,3000,3600,4500]; A0 = [0,600,1200,2000,2600,3000,3400,3400]
    print("curve", [(r, curve(r, R, A0)) for r in (650,1000,1799,2100,3300)])
    print("seg", segment_delay_us(3333,5000,0,1234,7))
    for rod in (150,165,178,190,200):
        print("rod %d mm: 1/2in -> %.2f deg, 7/16in -> %.2f deg" %
              (rod, piston_travel_to_deg(12.7, rod_mm=rod), piston_travel_to_deg(11.1125, rod_mm=rod)))
