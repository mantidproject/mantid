#############################################################################################
#
# Example script to demonstrate the use of MPI in Mantid.
# This requires the mpi4py python bindings and obviously an MPI-enabled Mantid build.
#
#############################################################################################

from MantidFramework import *
from mpi4py import MPI
from mantidsimple import *
mtd.initialise()

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

print "Running on rank %d of %d" % (rank, size)

eventfile="../../../../../Test/AutoTestData/CNCS_7860_event.nxs"
wksp="partial"
binning="40e3, 100, 70e3"

Load(Filename=eventfile, OutputWorkspace=wksp)

# Find which chunk to process
w = mtd[wksp]
chunk = w.getNumberHistograms() / size
startWI = rank*chunk
endWI = startWI + chunk - 1

# Do some processing (something silly for now)
SumSpectra(InputWorkspace=wksp, OutputWorkspace=wksp, StartWorkspaceIndex="%d"%startWI, EndWorkspaceIndex="%d"%endWI)
Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=binning)


# BroadcastWorkspace(InputWorkspace=van,OutputWorkspace="Vanadium",BroadcasterRank=len(runs))
GatherWorkspaces(InputWorkspace=wksp, OutputWorkspace="total")

if rank == 0:
    SaveNexus(InputWorkspace="total",Filename="mpi.nxs")

