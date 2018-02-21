

import sys, argparse
import solver

    
solver = solver.solver()

parser = argparse.ArgumentParser()

parser.add_argument("-x", "--width", type=int, default=solver.NX)
parser.add_argument("-y", "--height", type=int, default=solver.NY)
parser.add_argument("-d", "--directory", type=str, default="../restarts/")
parser.add_argument("-i", "--iterations", type=int, required=True)
parser.add_argument("-s", "--step", type=int, required=True)
parser.add_argument("-c", "--cfl", type=float, default=solver.CFL)
parser.add_argument("-t", "--td", type=float, default=solver.TD)

args = parser.parse_args()


solver.NX = args.width
solver.NY = args.height
solver.CFL = args.cfl
solver.TD = args.td
step = args.step
it = args.iterations
solver.restart_dir = args.directory



solver.create_restart_files(0, it, step)       


