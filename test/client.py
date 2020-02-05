import ctypes as c
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as anim
import time
import datetime
import astropy.io.fits as pf
import socket
import sys

import collections

from matplotlib import rc
rc('font',**{'family':'sans-serif','sans-serif':['Arial'],'size':10})
## for Palatino and other serif fonts use:
#rc('font',**{'family':'serif','serif':['Palatino']})
#rc('text', usetex=True)

if len(sys.argv)<2:
    print("Invocation: python client.py <Server IP>")
    sys.exit()

ip = sys.argv[1]

def timenow():
    return int((datetime.datetime.now().timestamp()*1e3))

class packet_data(c.Structure):
    _fields_ = [
        ('x_B',c.c_double),
        ('y_B',c.c_double),
        ('z_B',c.c_double),
        ('x_Bt',c.c_double),
        ('y_Bt',c.c_double),
        ('z_Bt',c.c_double),
        ('x_W',c.c_double),
        ('y_W',c.c_double),
        ('z_W',c.c_double)
    ]

port = 12376

SH_BUFFER_SIZE = 500

x_B = collections.deque(maxlen=SH_BUFFER_SIZE)
y_B = collections.deque(maxlen=SH_BUFFER_SIZE)
z_B = collections.deque(maxlen=SH_BUFFER_SIZE)

x_Bt = collections.deque(maxlen=SH_BUFFER_SIZE)
y_Bt = collections.deque(maxlen=SH_BUFFER_SIZE)
z_Bt = collections.deque(maxlen=SH_BUFFER_SIZE)

x_W = collections.deque(maxlen=SH_BUFFER_SIZE)
y_W = collections.deque(maxlen=SH_BUFFER_SIZE)
z_W = collections.deque(maxlen=SH_BUFFER_SIZE)

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

#print(c.sizeof(packet_data))
fig, (ax1, ax2, ax3) = plt.subplots(3,1,figsize=(10,8),sharex=True)
fig.suptitle("Timestamp: %s"%(datetime.datetime.fromtimestamp(timenow()//1e3)))

plt.xlabel("time (s)")
# intialize two line objects (one in each axes)
x_l_B, = ax1.plot([], [], color='r')
y_l_B, = ax1.plot([], [], color='b')
z_l_B, = ax1.plot([], [], color='g')

x_l_Bt, = ax2.plot([], [], color='r')
y_l_Bt, = ax2.plot([], [], color='b')
z_l_Bt, = ax2.plot([], [], color='g')

x_l_W, = ax3.plot([], [], color='r')
y_l_W, = ax3.plot([], [], color='b')
z_l_W, = ax3.plot([], [], color='g')

# all data plots
line = [x_l_B, y_l_B, z_l_B, x_l_Bt, y_l_Bt, z_l_Bt, x_l_W, y_l_W, z_l_W]
# vertical marker
# vline = []
for ax in [ax1, ax2, ax3]:
    ax.grid()
#     vline.append(ax.axvline(0, color='k'))

# Set limits
ax1.set_title("B (mG)")
ax1.set_ylim(-65e-6*1e7,65e-6*1e7) # mag field in mG

ax2.set_title("dB/dt (mG s^-1)")
ax2.set_ylim(-500,500) # B dot in mG s^-1

ax3.set_title("ω (rad s^-1)")
ax3.set_ylim(-1,1) # 1 rad s^-1

a = packet_data() 

def animate(i):
    # Read packet over network
    global a
    fig.suptitle("Timestamp: %s"%(datetime.datetime.fromtimestamp(timenow()//1e3)))
    val = ''.encode('utf-8')
    #print("Receiving %d packets:"%(1))
    for j in range(1):
        s = socket.socket()
        temp = ''.encode('utf-8')
        #print("Socket created successfully")
        while True:
            try:
                s.connect((ip,port))
                break
            except Exception:
                continue
        try:
            temp = s.recv(c.sizeof(packet_data),socket.MSG_WAITALL)
        except Exception:
            pass
        if (len(temp)!=c.sizeof(packet_data)):
            print("Received: ",len(val))
        s.close()
        val += temp
    # copy read bytes to the packet
    c.memmove(c.addressof(a),val,c.sizeof(packet_data))

    # Time axis generation
    xdata = np.arange(SH_BUFFER_SIZE, dtype = float) + i - SH_BUFFER_SIZE # time in seconds
    xdata *= 0.1
    # set time axis limits
    for ax in [ax1,ax2,ax3]:
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
    # update line
    line = [x_l_B, y_l_B, z_l_B, x_l_Bt, y_l_Bt, z_l_Bt, x_l_W, y_l_W, z_l_W]
    return line


animator = anim.FuncAnimation(fig,animate,blit=False,repeat=False,interval=50)
plt.show()
print("\n")