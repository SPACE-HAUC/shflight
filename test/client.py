import ctypes as c
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as anim
from matplotlib import gridspec
import time
import datetime
import astropy.io.fits as pf
import socket
import sys

import collections

from matplotlib import rc
rc('font', **{'family': 'sans-serif', 'sans-serif': ['Arial'], 'size': 10})
# for Palatino and other serif fonts use:
# rc('font',**{'family':'serif','serif':['Palatino']})
#rc('text', usetex=True)

plt.rcParams['figure.constrained_layout.use'] = True

if len(sys.argv) < 2:
    print("Invocation: python client.py <Server IP>")
    sys.exit()

ip = sys.argv[1]


def timenow():
    return int((datetime.datetime.now().timestamp()*1e3))


class packet_data(c.Structure):
    _fields_ = [
        ('mode', c.c_uint8),
        ('step', c.c_uint64),
        ('x_B', c.c_float),
        ('y_B', c.c_float),
        ('z_B', c.c_float),
        ('x_Bt', c.c_float),
        ('y_Bt', c.c_float),
        ('z_Bt', c.c_float),
        ('x_W', c.c_float),
        ('y_W', c.c_float),
        ('z_W', c.c_float),
        ('x_S', c.c_float),
        ('y_S', c.c_float),
        ('z_S', c.c_float)
    ]


port = 12376

SH_BUFFER_SIZE = 512 # Updated to get 2^9 size for ease of FFT

x_B = collections.deque(maxlen=SH_BUFFER_SIZE)
y_B = collections.deque(maxlen=SH_BUFFER_SIZE)
z_B = collections.deque(maxlen=SH_BUFFER_SIZE)

x_Bt = collections.deque(maxlen=SH_BUFFER_SIZE)
y_Bt = collections.deque(maxlen=SH_BUFFER_SIZE)
z_Bt = collections.deque(maxlen=SH_BUFFER_SIZE)

x_W = collections.deque(maxlen=SH_BUFFER_SIZE)
y_W = collections.deque(maxlen=SH_BUFFER_SIZE)
z_W = collections.deque(maxlen=SH_BUFFER_SIZE)

theta = collections.deque(maxlen=SH_BUFFER_SIZE)
phi = collections.deque(maxlen=SH_BUFFER_SIZE)

dang = collections.deque(maxlen=SH_BUFFER_SIZE)

x_S = collections.deque(maxlen=SH_BUFFER_SIZE)
y_S = collections.deque(maxlen=SH_BUFFER_SIZE)
z_S = collections.deque(maxlen=SH_BUFFER_SIZE)

x_sang = collections.deque(maxlen=SH_BUFFER_SIZE)
y_sang = collections.deque(maxlen=SH_BUFFER_SIZE)

for i in range(SH_BUFFER_SIZE):
    x_B.append(0)
    y_B.append(0)
    z_B.append(0)

    x_Bt.append(0)
    y_Bt.append(0)
    z_Bt.append(0)

    x_W.append(0)
    y_W.append(0)
    z_W.append(0)

    theta.append(0)
    phi.append(0)

    dang.append(0)

    x_S.append(0)
    y_S.append(0)
    z_S.append(0)

    x_sang.append(0)
    y_sang.append(0)

# print(c.sizeof(packet_data))
fig = plt.figure(constrained_layout=True)
spec = gridspec.GridSpec(ncols=5, nrows=6, figure=fig)
ax1 = fig.add_subplot(spec[0, 0:3])
ax2 = fig.add_subplot(spec[1, 0:3], sharex=ax1)
ax3 = fig.add_subplot(spec[2, 0:3], sharex=ax1)
ax4 = fig.add_subplot(spec[3, 0:3], sharex=ax1)
ax5 = fig.add_subplot(spec[4, 0:3], sharex=ax1)
ax6 = fig.add_subplot(spec[5, 0:3], sharex=ax1)
ax7 = fig.add_subplot(spec[:, 3:5])
# fig, (ax1, ax2, ax3, ax4, ax5, ax7) = plt.subplots(6,1,figsize=(10,8),sharex=True)
fig.suptitle("Timestamp: %s" %
             (datetime.datetime.fromtimestamp(timenow()//1e3)))

ax6.set_xlabel("time (s)")
# intialize two line objects (one in each axes)
x_l_B, = ax1.plot([], [], color='r', label='x')
y_l_B, = ax1.plot([], [], color='b', label='y')
z_l_B, = ax1.plot([], [], color='k', label='z')

x_l_Bt, = ax2.plot([], [], color='r', label='x')
y_l_Bt, = ax2.plot([], [], color='b', label='y')
z_l_Bt, = ax2.plot([], [], color='k', label='z')

x_l_W, = ax3.plot([], [], color='r', label='x')
y_l_W, = ax3.plot([], [], color='b', label='y')
z_l_W, = ax3.plot([], [], color='k', label='z')

l_theta, = ax4.plot([], [], color='r', label='θ')
l_phi, = ax4.plot([], [], color='b', label='φ')

l_dang, = ax5.plot([], [], color='k', label='ω · B')

x_l_sang, = ax6.plot([], [], color='r', label='x')
y_l_sang, = ax6.plot([], [], color='b', label='y')

x_l_B_fft, = ax7.plot([], [], color='r', label='FFT(Bx)')
y_l_B_fft, = ax7.plot([], [], color='b', label='FFT(By)')
z_l_B_fft, = ax7.plot([], [], color='k', label='FFT(Bz)')

ax1.legend()
ax2.legend()
ax3.legend()
ax4.legend()
ax5.legend()
ax6.legend()
ax7.legend()

# vertical marker
# vline = []
for ax in [ax1, ax2, ax3, ax4, ax5, ax6, ax7]:
    ax.grid()
#     vline.append(ax.axvline(0, color='k'))

# Set limits
ax1.set_title("B (mG)")
ax1.set_ylim(-65e-6*1e7, 65e-6*1e7)  # mag field in mG

ax2.set_title("dB/dt (mG s^-1)")
ax2.set_ylim(-500, 500)  # B dot in mG s^-1

w_min = -0.25
w_max = 0.25
ax3.set_title("ω (rad s^-1)")
ax3.set_ylim(w_min, w_max)  # 1 rad s^-1

ang_min = -90
ang_max = 90
ax4.set_title("Angles with axis (°)")
ax4.set_ylim(ang_min, ang_max)

dang_min = -90
dang_max = 90
ax5.set_title("Angle of ω with B (°)")
ax5.set_ylim(dang_min, dang_max)

sang_min = -90
sang_max = 90
ax6.set_title("Sun vector")
ax6.set_ylim(sang_min, sang_max)

ax7.set_title("FFT(B)")
ax7.set_xlim(-0.01, 5*0.5/np.pi)  # in Hz; 1 rad/s = 0.16 Hz
ax7.set_xlabel('Freq (rad/s)')
vline = []
cols = ['r', 'b', 'g']
lss = ['-', '-.', ':']
for i in range(3):
    vline.append(ax7.axvline(0, color=cols[i], ls=lss[i]))

# all data plots
line = [x_l_B, y_l_B, z_l_B, x_l_Bt, y_l_Bt, z_l_Bt, x_l_W, y_l_W, z_l_W, l_theta,
        l_phi, l_dang, x_l_sang, y_l_sang, x_l_B_fft, y_l_B_fft, z_l_B_fft, vline[0], vline[1], vline[2]]

a = packet_data()

acs_ct = 0

acs_mode = ["Detumble", "Sunpoint", "Night", "Ready"]
def animate(i):
    # Read packet over network
    global a, acs_ct, w_min, w_max, ang_min, ang_max, vline
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

    fig.suptitle("Timestamp: %s, Mode: %s" %
                 (datetime.datetime.fromtimestamp(timenow()//1e3),acs_mode[a.mode]))
    # Time axis generation
    if i == 0:
        acs_ct = a.step
    xdata = np.arange(SH_BUFFER_SIZE, dtype=float) + acs_ct + \
        i - SH_BUFFER_SIZE  # time in seconds
    xdata *= 0.1
    # set time axis limits
    for ax in [ax1, ax2, ax3, ax4, ax5]:
        ax.set_xlim(xdata.min(), xdata.max())
    # copy data into circular buffer
    x_B.append(a.x_B)
    y_B.append(a.y_B)
    z_B.append(a.z_B)
    x_Bt.append(a.x_Bt)
    y_Bt.append(a.y_Bt)
    z_Bt.append(a.z_Bt)
    x_W.append(a.x_W)
    y_W.append(a.y_W)
    z_W.append(a.z_W)
    x_S.append(a.x_S)
    y_S.append(a.y_S)
    z_S.append(a.z_S)
    w_norm = np.sqrt(a.x_W**2 + a.y_W**2 + a.z_W**2)
    B_norm = np.sqrt(a.x_B**2 + a.y_B**2 + a.z_B**2)
    S_norm = np.sqrt(a.x_S**2 + a.y_S**2 + a.z_S**2)
    Bx = a.x_B/B_norm
    By = a.y_B/B_norm
    Bz = a.z_B/B_norm
    Wx = a.x_W/w_norm
    Wy = a.y_W/w_norm
    Wz = a.z_W/w_norm
    ang = 180/np.pi*np.arccos(Bx*Wx + By*Wy + Bz*Wz)
    dang.append(ang)
    if w_norm > 0:
        theta.append(180/np.pi * np.arccos(a.z_W/w_norm))
        phi.append(180/np.pi * np.arctan2(a.y_W, a.x_W))
    else:
        theta.append(0)
        phi.append(0)
    s_valid = False
    s_dir = 1 if a.z_S > 0 else -1
    if S_norm > 0 and a.z_S != 0: # valid sun vector
        s_valid = True
        x_sang.append(180/np.pi * np.arctan2(a.x_S, a.z_S))
        y_sang.append(180/np.pi * np.arctan2(a.y_S, a.z_S))

    # Change limits for B_dot
    Bt_min = (np.array([np.min(x_Bt), np.min(y_Bt), np.min(z_Bt)])).min()
    Bt_min -= np.abs(Bt_min) * 0.1  # 10%
    Bt_max = (np.array([np.max(x_Bt), np.max(y_Bt), np.max(z_Bt)])).max()
    Bt_max += np.abs(Bt_max) * 0.1  # 10%

    ax2.set_ylim(Bt_min, Bt_max)

    # Change limits for W
    W_min = (np.array([np.min(x_W), np.min(y_W), np.min(z_W)])).min()
    W_min -= np.abs(W_min) * 0.1  # 10%
    W_max = (np.array([np.max(x_W), np.max(y_W), np.max(z_W)])).max()
    W_max += np.abs(W_max) * 0.1  # 10%

    w_min = W_min if W_min < w_min else w_min
    w_max = W_max if W_max > w_max else w_max

    ax3.set_ylim(w_min, w_max)
    ax3.set_title("ω (rad s^-1); ω_z = %0.3f rad s^-1" % (a.z_W))

    # Change limits for angle
    ang_min = (np.array([np.min(theta), np.min(phi)])).min()
    ang_min -= np.abs(ang_min) * 0.1
    ang_max = (np.array([np.max(theta), np.max(phi)])).max()
    ang_max += np.abs(ang_max) * 0.1

    ax4.set_ylim(ang_min, ang_max)

    ax4.set_title(("Angles with axis (°); ω · z = %.5f°" %
                   (np.average(theta))))

    # Change limits for B angle
    dang_min = np.min(dang)
    dang_min -= np.abs(dang_min) * 0.1
    dang_max = np.max(dang)
    dang_max += np.abs(dang_max) * 0.1

    ax5.set_ylim(dang_min, dang_max)
    ax5.set_title(("Angles with Magnetic Field (°); ω · B = %.5f°" % (ang)))

    # Set title for Sun vector
    ax6.set_title("Sun vector, status: %d | Z: %d, X: %.3f°, Y: %.3f°"%(s_valid, s_dir, x_sang[SH_BUFFER_SIZE-1], y_sang[SH_BUFFER_SIZE-1]))

    #print(np.real(np.fft.fftshift(np.fft.rfftn(x_B, norm='ortho'))).shape, xdata.shape)
    x_B_fft = np.abs(np.fft.fftshift(np.fft.fft(x_B, norm='ortho')))
    y_B_fft = np.abs(np.fft.fftshift(np.fft.fft(y_B, norm='ortho')))
    z_B_fft = np.abs(np.fft.fftshift(np.fft.fft(z_B, norm='ortho')))

    # time gap = 0.1 s, 10x to account for frequency mismatch in fft
    fft_base = 10*0.5/np.pi * \
        np.fft.fftshift(np.fft.fftfreq(SH_BUFFER_SIZE, 0.1))

    #vx = fft_base[np.where(x_B_fft[np.where(fft_base>=0)]==x_B_fft[np.where(fft_base>=0)].max())]
    # print(vx)
    vx = (fft_base[np.where(fft_base >= 0)][np.where(
        x_B_fft[np.where(fft_base >= 0)] == x_B_fft[np.where(fft_base >= 0)].max())])[0]
    vy = (fft_base[np.where(fft_base >= 0)][np.where(
        y_B_fft[np.where(fft_base >= 0)] == y_B_fft[np.where(fft_base >= 0)].max())])[0]
    vz = (fft_base[np.where(fft_base >= 0)][np.where(
        z_B_fft[np.where(fft_base >= 0)] == z_B_fft[np.where(fft_base >= 0)].max())])[0]

    vmax = np.array([vx, vy, vz]).max()
    if vmax < 1:
        vmax = 1
    elif 1 < vmax <= 2:
        vmax = 2
    elif 2 < vmax <= 4:
        vmax = 4
    else:
        vmax = SH_BUFFER_SIZE*0.1*0.5/np.pi
    # Change limits for B
    B_fft_min = (
        np.array([np.min(x_B_fft), np.min(y_B_fft), np.min(z_B_fft)])).min()
    B_fft_min -= np.abs(B_fft_min) * 0.1  # 10%
    B_fft_max = (
        np.array([np.max(x_B_fft), np.max(y_B_fft), np.max(z_B_fft)])).max()
    B_fft_max += np.abs(B_fft_max) * 0.1  # 10%

    ax7.set_ylim(B_fft_min, B_fft_max)
    ax7.set_xlim(-0.01, vmax)
    ax7.set_title(
        "Freq X: %.3f rad/s, Y: %.3f rad/s, Z: %.3f rad/s" % (vx, vy, vz))

    # plot current data in color
    x_l_B.set_data(xdata, x_B)
    y_l_B.set_data(xdata, y_B)
    z_l_B.set_data(xdata, z_B)

    x_l_Bt.set_data(xdata, x_Bt)
    y_l_Bt.set_data(xdata, y_Bt)
    z_l_Bt.set_data(xdata, z_Bt)

    x_l_W.set_data(xdata, x_W)
    y_l_W.set_data(xdata, y_W)
    z_l_W.set_data(xdata, z_W)

    l_theta.set_data(xdata, theta)
    l_phi.set_data(xdata, phi)

    l_dang.set_data(xdata, dang)

    x_l_sang.set_data(xdata, x_sang)
    y_l_sang.set_data(xdata, y_sang)

    x_l_B_fft.set_data(fft_base, x_B_fft)
    y_l_B_fft.set_data(fft_base, y_B_fft)
    z_l_B_fft.set_data(fft_base, z_B_fft)

    vline[0].set_data([vx, vx], [B_fft_min-1e3, B_fft_max+1e3])
    vline[1].set_data([vy, vy], [B_fft_min-1e3, B_fft_max+1e3])
    vline[2].set_data([vz, vz], [B_fft_min-1e3, B_fft_max+1e3])
    # update line
    line = [x_l_B, y_l_B, z_l_B, x_l_Bt, y_l_Bt, z_l_Bt, x_l_W, y_l_W, z_l_W, l_theta,
            l_phi, l_dang, x_l_B_fft, y_l_B_fft, z_l_B_fft, vline[0], vline[1], vline[2]]
    return line


animator = anim.FuncAnimation(
    fig, animate, blit=False, repeat=False, interval=100)
plt.show()
print("\n")
