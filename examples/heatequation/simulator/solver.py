#Python port of http://www.tonysaad.net/code/two-dimensional-heat-equation-in-c/

from netCDF4 import Dataset
import time


class heatstep:    
    def __init__(self, it, filename):
        self.iteration=it
        self.filename = filename

class solver:
    CFL=0.25
    NX=100
    NY=100
    TD=0.5
    
    rightBC=20.0
    leftBC=20.0
    topBC=20.0
    bottomBC=20.0
    
    deltaX=1.0/NX
    deltaY=1.0/NY
    deltaX2=deltaX*deltaX
    deltaY2=deltaY*deltaY
    deltaT=CFL*(deltaX2*deltaY2)/((deltaX2+deltaY2)*TD)

    restart_dir = "../restarts/"
    result_dir = "../output/"

    current_it = 0
    restart_file = None
    grid = None

    def __init__(self):
        self.grid = self.initialMatrix()

    
    def initVal(self, i, j):
        if (i==0 and j==0): return 0.5*(self.leftBC+self.bottomBC)
        if (i==0 and j==self.NY-1): return 0.5*(self.topBC+self.leftBC)
        if (i==self.NX-1 and j==self.NY-1): return 0.5*(self.topBC+self.rightBC)
        if (i==self.NX-1 and j==0): return 0.5*(self.bottomBC+self.rightBC)
        if (j==0): return self.bottomBC;
        if (j==self.NY-1): return self.topBC;
        if (i==0): return self.leftBC;
        if (i==self.NX-1): return self.rightBC;
        return 0.0


    def initialMatrix(self):
        #create 0 matrix
        return [[self.initVal(i, j) for j in range(0, self.NY)] for i in range(0, self.NX)]
                  
    def f(self, i, j): 
        old = self.grid
        if (i==0 or i==self.NX-1 or j==0 or j==self.NY-1): return old[i][j]
        return old[i][j] + self.CFL*(old[i+1][j] + old[i-1][j] - 4*old[i][j] + old[i][j-1] + old[i][j+1])

    def compute_step(self):
        mnew = [[self.f(i, j) for j in range(0, self.NY)] for i in range(0, self.NX)]
        self.grid = [row[:] for row in mnew]
        self.write_to_disk()
        self.current_it = self.current_it+1

    

    def restart(self, restartfile):
        self.grid, self.current_it = self.load_from_disk(restartfile)


    def compute(self, stop_iteration, sleep=0):

        res = []

        if (self.current_it==stop_iteration): 
            return [heatstep(self.current_it, self.get_filename())]
        while (self.current_it<=stop_iteration):
            self.compute_step()    
            res.append(heatstep(self.current_it, self.get_filename()))
            time.sleep(sleep)


        if (len(res)==0):
            print "ERROR: " + str(self.current_it) + "; " + str(stop_iteration)
        #print "solver: iterations: %i" %(count) 
        #return heatstep(stop_iteration, get_filename())
        return res

    def get_filename(self): return "data_" + str(self.current_it)


    def netCDF_open(self, fname, mode):
        root = Dataset(fname, mode, format="NETCDF4")

        if (mode=="w"):
            xdim = root.createDimension("nx", self.NX)
            ydim = root.createDimension("ny", self.NY)
            tdim = root.createDimension("time", None)

            data = root.createVariable("data", 'f8', ('nx', 'ny')) 
        else:
            data = root.variables["data"]

        return root, data;

    def netCDF_close(self, dataset):
        dataset.close()

    def load_from_disk(self, fname):
        data=[]
        dataset, rdata = self.netCDF_open(fname, "r");
        data[:] = rdata
        it = dataset.iteration
        self.netCDF_close(dataset);
        return data, it

    #write the current iteration to disk
    def write_to_disk(self):
        dataset, data = self.netCDF_open(self.result_dir + self.get_filename(), "w");
        dataset.iteration = self.current_it;
        data[:] = self.grid
        self.netCDF_close(dataset);

    #the restart file is just an result file with some more data
    def write_restart_file(self):
        print("writing restart: " + self.restart_dir + "restart_" + str(self.current_it))
        dataset, data = self.netCDF_open(self.restart_dir + "restart_" + str(self.current_it), "w")
        dataset.nx = self.NX
        dataset.ny = self.NY
        dataset.cfl = self.CFL
        dataset.td = self.TD
        dataset.iteration = self.current_it
        data[:] = self.grid

    #restart files are equal to result file in this case
    def create_restart_files(self, start, stop, interval):
        for x in range(start, stop):
            self.compute_step()
            if ((x + 1) % interval==0): self.write_restart_file()
    


#params = solver_parameters()

#res = compute(params, 100, "restart/restart_57")
#write_to_disk("test.nc", res.data, 100);

#create_restart_files(params, 0, 100, 10);



