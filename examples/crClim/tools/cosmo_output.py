import f90nml
import sys
import os
from datetime import datetime, timedelta

if (len(sys.argv)!= 2):
    print("Usage: %s <INPUT_* path>\n" % (sys.argv[0])) 
    exit(1)

ipath = sys.argv[1]

nml_io = f90nml.read(ipath + "/INPUT_IO")
nml_org = f90nml.read(ipath + "/INPUT_ORG")


start_date_str = nml_org["RUNCTL"]["ydate_ini"]
start_date = datetime.strptime(start_date_str, "%Y%m%d%H%M%S")
timestep = nml_org["RUNCTL"]["dt"]

gribout = nml_io["gribout"]

#print("Start date: " + str(start_date))


for out in gribout:
    #print(out["ydir"])
    #print(out["hcomb"])
    #print(out["ytunit"])

    
    gribpath = os.path.abspath(ipath + out["ydir"])

    if ("hcomb" in out):
        grib_current = start_date + timedelta(hours = out["hcomb"][0])
        grib_stop = start_date + timedelta(hours = out["hcomb"][1])
        grib_incr = timedelta(hours = out["hcomb"][2])
    elif("ncomb" in out):
        grib_current = start_date + timedelta(seconds = float(out["ncomb"][0])*timestep)
        grib_stop = start_date + timedelta(seconds = float(out["ncomb"][1])*timestep)
        grib_incr = timedelta(seconds = timestep*out["ncomb"][2])
    else:
        print("Cannot find hcomb or ncomb... aborting!")
        exit(1)




    if (out["ytunit"] == "d"):
    
        while(grib_current <= grib_stop):
            print("%s/lff%s%s.nc" %(gribpath, out["ytunit"], grib_current.strftime("%Y%m%d%H%M%S")))
            grib_current = grib_current + grib_incr
    elif (out["ytunit"] == "f"):
               
        while (grib_current <= grib_stop):
            grib_delta = (grib_current - start_date)
            grib_delta_s = grib_delta.seconds
            d = grib_delta.days
            h, r = divmod(grib_delta_s, 3600) 
            m, s = divmod(r, 60)       

            print("%s/lff%s%s%s%s%s.nc" %(gribpath, out["ytunit"], str(d).zfill(2), str(h).zfill(2), str(m).zfill(2), str(s).zfill(2)))
            grib_current = grib_current + grib_incr
    else:
        print("ytunit not recognized... aborting!")
        exit(1)



    #print("----")


