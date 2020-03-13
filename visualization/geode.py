from mpl_toolkits.basemap import Basemap
import ctypes as c
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as anim
from matplotlib import gridspec
from mpl_toolkits.mplot3d import Axes3D
import time
import datetime
import socket
import sys
import collections
from matplotlib import rc
rc('font', **{'family': 'sans-serif', 'sans-serif': ['Arial'], 'size': 10})

plt.rcParams['figure.constrained_layout.use'] = True

if len(sys.argv) < 2:
    print("Invocation: python client.py <Server IP>")
    sys.exit()

ip = sys.argv[1]

port = 12380


def timenow():
    return int((datetime.datetime.now().timestamp()*1e3))


def normalize(input):
    norm = np.sqrt(np.sum(input*input))
    out = np.zeros(3, dtype=float)
    if norm != 0:
        out = input / norm
    return (out, norm)


class packet_data(c.Structure):
    _fields_ = [
        ('lat', c.c_float),
        ('lon', c.c_float),
        ('alt', c.c_float),

        ('x_B', c.c_float),
        ('y_B', c.c_float),
        ('z_B', c.c_float),

        ('x_W', c.c_float),
        ('y_W', c.c_float),
        ('z_W', c.c_float),

        ('x_S', c.c_float),
        ('y_S', c.c_float),
        ('z_S', c.c_float),

        ('x_E', c.c_float),
        ('y_E', c.c_float),
        ('z_E', c.c_float),

        ('x_T', c.c_float),
        ('y_T', c.c_float),
        ('z_T', c.c_float),

        ('x_Td', c.c_float),
        ('y_Td', c.c_float),
        ('z_Td', c.c_float),

        ('batt_level', c.c_float)
    ]


a = packet_data()

# my_map = Basemap(projection='ortho', resolution=None, lat_0=0, lon_0=-100)
# my_map.bluemarble(scale=0.5)
lats = collections.deque(maxlen=90*3600)
lons = collections.deque(maxlen=90*3600)

bat = collections.deque(maxlen=6000)  # 10 minutes worth of data

for i in range(6000):
    bat.append(0)

fig = plt.figure(figsize=(10, 10), constrained_layout=True)
fig.canvas.set_window_title("Timestamp: %s" % (str(datetime.datetime.now())))

spec = gridspec.GridSpec(ncols=1, nrows=14, figure=fig)

ax1 = fig.add_subplot(spec[0:6, :])  # map
ax2 = fig.add_subplot(spec[6:11, :], projection='3d')  # orientations
ax3 = fig.add_subplot(spec[12:14, :])  # power meter

ax1.set_title("Lat: %.2f %s, Lon: %.2f %s, Alt: %.2f km" % (0, '', 0, '', 0))
ax1.grid()

ax3.set_title(
    "ω_z: %.3f rad/s, ω·z: %.3f°, S·z: %.3f°, E·z: %.3f°, T_d: %.3f N-m\nBattery level: %.3f W-h" % (0, 0, 0, 0, 0, 0))
ax2.grid()
# ax2.set_aspect('equal')

ax2.set_xlim(-2, 2)
ax2.set_ylim(-2, 2)
ax2.set_zlim(-2, 2)

ax2.set_xlabel("X")
ax2.set_ylabel("Y")
ax2.set_zlabel("Z")

my_map = Basemap(projection='cyl', resolution='i',  # intermediate resolution
                 llcrnrlat=-90, urcrnrlat=90,
                 llcrnrlon=-180, urcrnrlon=180, ax=ax1,)
my_map.shadedrelief(scale=0.1)

x, y = my_map(0, 0)  # position of the satellite
point = my_map.plot(x, y, 'ro', markersize=5)[0]
line, = ax1.plot([], [],)  # trail of the satellite

objs = []
colors = ['k', 'r', 'y', 'm', 'c', 'g']
ident = ['B', 'W', 'S', 'E', 'T', 'T_d']
objs.append(point)
objs.append(line)

for i in range(6):
    objs.append(ax2.quiver(0, 0, 0, 0, 0, 0, color=colors[i], label=ident[i]))
txts = []
for i in range(6):
    tmt = ax2.text(0, 0, 0, ident[i])
    txts.append(tmt)
plt.legend()
bat_line, = ax3.plot([], [],)
objs.append(bat_line)

bat_lo = -40
bat_hi = 40
ax3.grid()
ax3.set_ylim(bat_lo, bat_hi)
ax3.set_xlim(0, 599)

plt.legend()

# def init():
#     global objs
#     objs[0].set_data([], [])
#     objs[1].set_data([], [])
#     return objs,

o_lats = 0
o_lons = 0
_lats = 0
_lons = 0

# animation function.  This is called sequentially


def animate(i):
    global a, o_lats, o_lons, _lats, _lons, objs, colors, ident, txts
    val = ''.encode('utf-8')
    #print("Receiving %d packets:"%(1))
    for j in range(1):
        s = socket.socket()
        temp = ''.encode('utf-8')
        #print("Socket created successfully")
        while True:
            try:
                s.connect((ip, port))
                break
            except Exception:
                continue
        try:
            temp = s.recv(c.sizeof(packet_data), socket.MSG_WAITALL)
        except Exception:
            pass
        if (len(temp) != c.sizeof(packet_data)):
            print("Received: ", len(val))
        s.close()
        val += temp
    # copy read bytes to the packet
    c.memmove(c.addressof(a), val, c.sizeof(packet_data))

    o_lats = _lats
    o_lons = _lons
    _lats = a.lat
    _lons = a.lon
    # print(a.lat, a.lon, a.alt, a.x_W, a.y_W, a.z_W)
    vecs = [np.zeros(3) for i in range(6)]
    vecs[0][0] = a.x_B
    vecs[0][1] = a.y_B
    vecs[0][2] = a.z_B

    vecs[1][0] = a.x_W
    vecs[1][1] = a.y_W
    vecs[1][2] = a.z_W

    vecs[2][0] = a.x_S
    vecs[2][1] = a.y_S
    vecs[2][2] = a.z_S

    vecs[3][0] = a.x_E
    vecs[3][1] = a.y_E
    vecs[3][2] = a.z_E

    vecs[4][0] = a.x_T
    vecs[4][1] = a.y_T
    vecs[4][2] = a.z_T

    vecs[5][0] = a.x_Td
    vecs[5][1] = a.y_Td
    vecs[5][2] = a.z_Td

    # normalize vectors
    out, norm = normalize(vecs[0])
    vecs[0] = out
    print(vecs[0])
    vecs[4], T_norm = normalize(vecs[4])
    vecs[5], Td_norm = normalize(vecs[5])
    if T_norm > 0:
        vecs[5] *= Td_norm / T_norm

    x, y = my_map(_lons, _lats)
    if i > 0 and np.abs(_lons - o_lons) > 352:
        lons.append(np.nan)
    else:
        lons.append(_lons)
    lats.append(_lats)

    fig.canvas.set_window_title("Timestamp: %s" %
                                (str(datetime.datetime.now())))
    n_s = 'N' if _lats >= 0 else 'S'
    e_w = 'E' if _lons >= 0 else 'W'
    ax1.set_title("Lat: %.2f %s, Lon: %.2f %s, Alt: %.2f km" %
                  (_lats, n_s, _lons, e_w, a.alt/1000))
    point.set_data(x, y)
    line.set_data(lons, lats)

    omega_z = 180 * np.arccos(a.z_W)/np.pi
    S_z = 180 * np.arccos(vecs[2][2])/np.pi
    E_z = 180 * np.arccos(vecs[3][2])/np.pi

    for i in range(6):
        objs[2+i].remove()
        objs[2+i] = ax2.quiver(0, 0, 0, vecs[i][0], vecs[i]
                               [1], vecs[i][2], color=colors[i], label=ident[i])
        txts[i].remove()
        txts[i] = ax2.text(vecs[i][0], vecs[i][1], vecs[i][2], ident[i])

    batt_level = a.batt_level * 1e-3 / 3600  # milli Joules to W-h

    xr = np.arange(6000)

    bat.append(batt_level)
    ax3.set_title("ω_z: %.3f rad/s, ω·z: %.3f°, S·z: %.3f°, E·z: %.3f°, T_d: %.3f N-m\nBattery level: %.3f W-h" %
                  (a.z_W, omega_z, S_z, E_z, Td_norm, batt_level))
    objs[8].set_data(xr, bat)
    return objs
    # return point,


# call the animator.  blit=True means only re-draw the parts that have changed.
animator = anim.FuncAnimation(plt.gcf(), animate, interval=100, blit=False)
plt.show()
