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
import matplotlib.animation as animation

from netCDF4 import Dataset

i=0
surf=None

#1: forward; 0:backward
direction=1

#timestep
simstep=10

#animation flag
animating=False
stepone=False
data=None

#User interaction
def key_press_handler(event):
    global direction
    global animating
    global stepone
    global pause
    already_animating = animating
    print('press', event.key)
    if (event.key=='right'):
        print("right")
        direction=1
        stepone=True
    elif (event.key=='left'):
        print("left")
        direction=0
        stepone=True
    elif (event.key==' '):
        animating=not animating
    elif (event.key=="ctrl+c"):
        exit(1)

    


def get_data(filename):
    data = []
    print("Reading from " + filename)        
    ds = Dataset(filename, "r", format="NETCDF4")
    #if (ds.iteration!=it): print("ERROR: requested it: " + str(it) + "; read it: " + str(ds.iteration))
    data[:] = ds.variables["data"]
    ds.close()
    return data


def data_gen(framenumber):
    global animating
    global direction
    global i
    global stepone
    global data 

    nexti = i + 2*simstep*direction - simstep
   
    if (nexti>=0 and (animating or stepone or data==None)): 
        i = nexti
        data = get_data("../output/data_" + str(i))

    stepone = False
    ax.clear()

    x = np.arange(0, 100)
    y = np.arange(0, 100)
    xx, yy = np.meshgrid(x, y)

    surf = ax.plot_surface(xx, yy, data, cmap=cm.coolwarm, vmin=0, vmax=20)

    return surf



#init plot
fig = plt.figure()
ax = fig.gca(projection='3d')

#Labels
ax.set_xlabel('X Label')
ax.set_ylabel('Y Label')
ax.set_zlabel('Temperature')

ax.zaxis.set_major_locator(LinearLocator(10))
#surf = ax.plot_surface(xx, yy, data)
#xx,yy,data = update()



#install key handlers
fig.canvas.mpl_connect('key_press_event', key_press_handler)

#surf.set_clim(vmin=0, vmax=20)

#colorbar
#fig.colorbar(surf, shrink=0.5, aspect=5)

np.set_printoptions(precision=3)
np.set_printoptions(suppress=True)

plot_ani = animation.FuncAnimation(fig, data_gen, interval=10, blit=False)


#show window
plt.show()
