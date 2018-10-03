# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#############################################################################################
#
# Example script to demonstrate the use of MPI in Mantid.
# This requires the mpi4py python bindings and obviously an MPI-enabled Mantid build.
#
#############################################################################################

from mantid.simpleapi import *

def focus(eventfile, wksp, binning):
    calfile = "PG3_1370_2010_09_12.cal"
    Load(Filename=eventfile, OutputWorkspace=wksp)
    FilterBadPulses(InputWorkspace=wksp, OutputWorkspace=wksp)
    AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp,
                   CalibrationFile=calfile)
    DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,
                         GroupingFileName=calfile)
    Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=binning)
    SortEvents(InputWorkspace=wksp)
    NormaliseByCurrent(InputWorkspace=wksp,OutputWorkspace=wksp)
    return mtd[wksp]

##### Cooling data - Mn5Si3

runs = [2421, 2423, 2425, 2427, 2429]
binning = (1.4,-.0004,8.)

def mpimethod():
    from mpi4py import MPI
    world = MPI.COMM_WORLD

    if world.size != len(runs)+1:
        print "This script must be run with " + str(len(runs)+1) + " MPI processes!"
        from sys import exit
        exit()

    if world.rank == len(runs):
        van = focus("PG3_1875_event.nxs", "PG3_1875", binning)
    else:
        name = "PG3_%d" % runs[world.rank]
        data = focus(name + "_event.nxs", name, binning)
        van = None

    # MPI algorithms MUST be called by ALL processes in a job, or all the processes
    # that call it just hang (at least with OpenMPI).
    BroadcastWorkspace(InputWorkspace=van,OutputWorkspace="Vanadium",BroadcasterRank=len(runs))

    if world.rank < len(runs):
        van = mtd["Vanadium"]
        data /= van
    else:
        data = None

    # MPI algorithms MUST be called by ALL processes in a job, or all the processes
    # that call it just hang (at least with OpenMPI).
    GatherWorkspaces(InputWorkspace=data, OutputWorkspace="Mn5Si3")

    if world.rank == 0:
        SaveNexus(InputWorkspace="Mn5Si3",Filename="mpi.nxs")

def loopmethod():
    van = focus("PG3_1875_event.nxs", "PG3_1875", binning)
    names = []
    for run in runs:
        name = "PG3_%d" % run
        data = focus(name + "_event.nxs", name, binning)
        ConvertToMatrixWorkspace(data,data)
        data /= van
        names.append(str(data))

    for name in names[1:]:
            ConjoinWorkspaces(InputWorkspace1=names[0], InputWorkspace2=name,
                              CheckOverlapping=False,
                              OutputWorkspace=names[0])
            DeleteWorkspace(name)
    RenameWorkspace(InputWorkspace=names[0], OutputWorkspace="Mn5Si3")
    SaveNexus(InputWorkspace="Mn5Si3",Filename="nompi.nxs")

# Uncomment the method you want to run.
# On my machine, the mpi one takes ~9s, the single process method ~18s.
mpimethod()
#loopmethod()
