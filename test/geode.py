import ctypes as c
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as anim
from matplotlib import gridspec
import time
import datetime
import socket
import sys
import collections
from matplotlib import rc
rc('font', **{'family': 'sans-serif', 'sans-serif': ['Arial'], 'size': 10})
from mpl_toolkits.basemap import Basemap
import collections

plt.rcParams['figure.constrained_layout.use'] = True

if len(sys.argv) < 2:
    print("Invocation: python client.py <Server IP>")
    sys.exit()

ip = sys.argv[1]


def timenow():
    return int((datetime.datetime.now().timestamp()*1e3))

class geode_data(c.Structure):
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

        ('DCM00', c.c_float),
        ('DCM01', c.c_float),
        ('DCM02', c.c_float),
        ('DCM10', c.c_float),
        ('DCM11', c.c_float),
        ('DCM12', c.c_float),
        ('DCM20', c.c_float),
        ('DCM21', c.c_float),
        ('DCM22', c.c_float),

        ('x_T', c.c_float),
        ('y_T', c.c_float),
        ('z_T', c.c_float)
    ]


port = 12380

a = geode_data()

# my_map = Basemap(projection='ortho', resolution=None, lat_0=0, lon_0=-100)
# my_map.bluemarble(scale=0.5)
lats = collections.deque(maxlen=2*3600//30)
lons = collections.deque(maxlen=2*3600//30)

fig = plt.figure(figsize=(10,10),constrained_layout=True)
spec = gridspec.GridSpec(ncols=1, nrows=2, figure=fig)

ax1 = fig.add_subplot(spec[0, :])
ax2 = fig.add_subplot(spec[1, :],projection='3d')

ax2.grid()

ax2.set_xlim(-2,2)
ax2.set_ylim(-2,2)
ax2.set_zlim(-2,2)

my_map = Basemap(projection='cyl', resolution=None,
            llcrnrlat=-90, urcrnrlat=90,
            llcrnrlon=-180, urcrnrlon=180, ax=ax1,)
my_map.shadedrelief(scale=0.1)

x,y = my_map(0, 0)
point = my_map.plot(x, y, 'ro', markersize=5)[0]
line, = ax1.plot([], [],)

objs = []
colors = ['k', 'r', 'y', 'm', 'c']
objs.append(point)
objs.append(line)

for i in range(5):
    objs.append(ax2.quiver(0, 0, 0, 0, 0, 0, color = colors[i]))

def init():
    point.set_data([], [])
    line.set_data([], [])
    return objs,
o_lats = 0
o_lons = 0
_lats = 0
_lons = 0
# animation function.  This is called sequentially
def animate(i):
    global a, o_lats, o_lons, _lats, _lons
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
            temp = s.recv(c.sizeof(geode_data), socket.MSG_WAITALL)
        except Exception:
            pass
        if (len(temp) != c.sizeof(geode_data)):
            print("Received: ", len(val))
        s.close()
        val += temp
    # copy read bytes to the packet
    c.memmove(c.addressof(a), val, c.sizeof(geode_data))
    
    o_lats = _lats
    o_lons = _lons
    _lats = a.lat
    _lons = a.lon

    B = np.zeros(3)
    B[0] = a.x_B
    B[1] = a.y_B
    B[2] = a.z_B

    W = np.zeros(3)
    W[0] = a.x_W
    W[1] = a.y_W
    W[2] = a.z_W

    S = np.zeros(3)
    S[0] = a.x_S
    S[1] = a.y_S
    S[2] = a.z_S

    E = np.zeros(3)
    E[0] = a.x_E
    E[1] = a.y_E
    E[2] = a.z_E

    dcm = np.zeros((3,3),dtype=float)

    dcm[0][0] = a.DCM00
    dcm[0][1] = a.DCM01
    dcm[0][2] = a.DCM02
    dcm[1][0] = a.DCM10
    dcm[1][1] = a.DCM11
    dcm[1][2] = a.DCM12
    dcm[2][0] = a.DCM20
    dcm[2][1] = a.DCM21
    dcm[2][2] = a.DCM22

    S = dcm * S # S in body frame
    E = dcm * E # E in body frame

    x, y = my_map(_lons, _lats)
    if i > 0 and abs(a[2][i*30] - a[2][(i-1)*30]) > 352:
        lons.append(np.nan)
    else:
        lons.append(_lons)
    lats.append(_lats)
    point.set_data(x, y)
    line.set_data(lons, lats)
    return [point,line]
    # return point,

# call the animator.  blit=True means only re-draw the parts that have changed.
animator = anim.FuncAnimation(plt.gcf(), animate, init_func=init, interval=1, blit=True)

plt.show()