#Current key bindings:
#left/right arrow: change direction (forward/backword, respectively) and make a step in that direction
#space bar: start/stop the animation


#from Scientific.IO.NetCDF import NetCDFFile as Dataset
import numpy as np
from mpl_toolkits.mplot3d import Axes3D
from threading import Timer
from matplotlib import cm
from matplotlib.ticker import LinearLocator, FormatStrFormatter
import matplotlib.pyplot as plt

from netCDF4 import Dataset

i=0
surf=None

#1: forward; 0:backward
direction=1

#timestep
simstep=20

#animation flag
animating=False


#User interaction
def key_press_handler(event):
    global direction
    global animating
    print('press', event.key)
    if (event.key=='right'):
        print("right")
        direction=1
    elif (event.key=='left'):
        print("left")
        direction=0
    elif (event.key==' '):
        animating=not animating
    update()


def get_data(filename):
    data = []
    print("Reading from " + filename)        
    ds = Dataset(filename, "r", format="NETCDF4")
    #if (ds.iteration!=it): print("ERROR: requested it: " + str(it) + "; read it: " + str(ds.iteration))
    data[:] = ds.variables["data"]
    ds.close()
    return data


#last=None
def update():
    #print('press', event.key)
    global i
    global fig
    global surf
    global last
    global simstep
    global direction
    global animating

    #i = i + (direction)*simstep - (1-direction)*simstep 
    i = i + 2*simstep*direction - simstep

    if (i<=0):
        i=0 
        return

    #create x,y data (size of plate)
    x = np.arange(0, 100)
    y = np.arange(0, 100)

    #last = solver.compute(i, last);
    data = get_data("../output/data_" + str(i))

    #data = last.data


    print("update i:%i" % i)
    xx, yy = np.meshgrid(x, y)

    #print data
    ax.clear()

    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Temperature')

    #make the plot
    surf = ax.plot_surface(xx, yy, data, cmap=cm.coolwarm, vmin=0, vmax=20)

    #source box (cache/simulator)
    ctxt = "CACHE"
    ccolor = "green"
    fcolor = "black"

    #ax.text(-10, 8, 27, ctxt, bbox=dict(facecolor=ccolor, alpha=0.5, boxstyle='round,pad=1'), color=fcolor, fontweight='bold', fontsize=12, verticalalignment='center')


    ax.set_autoscale_on(False)
    ax.set_zlim(0,20)

    #fig.canvas.draw()
    plt.draw()

    #keep animating if animating==true
    if (animating): Timer(0.01, update).start()

    #now this is useless
    return (xx, yy, data)



#init plot
fig = plt.figure()
ax = fig.gca(projection='3d')

#Labels
ax.set_xlabel('X Label')
ax.set_ylabel('Y Label')
ax.set_zlabel('Temperature')

ax.zaxis.set_major_locator(LinearLocator(10))
#surf = ax.plot_surface(xx, yy, data)
xx,yy,data = update()

#install key handlers
fig.canvas.mpl_connect('key_press_event', key_press_handler)

surf.set_clim(vmin=0, vmax=20)

#colorbar
fig.colorbar(surf, shrink=0.5, aspect=5)

np.set_printoptions(precision=3)
np.set_printoptions(suppress=True)

#show window
plt.show()
