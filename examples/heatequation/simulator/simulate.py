import sys, argparse
import solver


solver = solver.solver()
parser = argparse.ArgumentParser()


parser.add_argument("-x", "--width", type=int, default=solver.NX)
parser.add_argument("-y", "--height", type=int, default=solver.NY)
parser.add_argument("-r", "--restartfile", type=str, default="")
parser.add_argument("-o", "--outputdir", type=str, default="../output/")
parser.add_argument("-i", "--endat", type=int, required=True)
parser.add_argument("-c", "--cfl", type=float, default=solver.CFL)
parser.add_argument("-t", "--td", type=float, default=solver.TD)

args = parser.parse_args()


solver.NX = args.width
solver.NY = args.height
solver.CFL = args.cfl
solver.TD = args.td
solver.result_dir = args.outputdir

if (args.restartfile!=""): solver.restart(args.restartfile)

solver.compute(args.endat)


