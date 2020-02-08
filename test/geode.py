import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.basemap import Basemap
import matplotlib.animation as animation
import collections
my_map = Basemap(projection='cyl', resolution=None,
            llcrnrlat=-90, urcrnrlat=90,
            llcrnrlon=-180, urcrnrlon=180, ) #ax=axes[1])
my_map.shadedrelief(scale=0.2)
#my_map.drawcoastlines()
#my_map.drawcountries()
#my_map.fillcontinents(color = 'gray')
#my_map.drawmapboundary()
#my_map.drawmeridians(np.arange(0, 360, 30))
#my_map.drawparallels(np.arange(-90, 90, 30))

# my_map = Basemap(projection='ortho', resolution=None, lat_0=0, lon_0=-100)
# my_map.bluemarble(scale=0.5)
lats = collections.deque(maxlen=180)
lons = collections.deque(maxlen=180)
x,y = my_map(0, 0)
point = my_map.plot(x, y, 'ro', markersize=5)[0]
line, = plt.plot([], [], marker='.', ls='')
dats = [point, line]
def init():
    point.set_data([], [])
    line.set_data([], [])
    return point,

# animation function.  This is called sequentially
def animate(i):
    _lons = i % 360 - 180 + (i // 360) * 360 / 45
    _lats = 67*np.sin(i*2*np.pi/180.0)
    # my_map = Basemap(projection='ortho', resolution=None, lat_0=lat_0, lon_0=lons)
    # my_map.bluemarble(scale=0.5)
    x, y = my_map(_lons, _lats)
    lats.append(_lats)
    lons.append(_lons)
    point.set_data(x, y)
    line.set_data(lons, lats)
    return [point,line]

# call the animator.  blit=True means only re-draw the parts that have changed.
anim = animation.FuncAnimation(plt.gcf(), animate, init_func=init, interval=5, blit=True)

plt.show()