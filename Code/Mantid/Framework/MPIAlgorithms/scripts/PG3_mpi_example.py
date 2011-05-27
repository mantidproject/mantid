#############################################################################################
#
# Example script to demonstrate the use of MPI in Mantid.
# This requires the boost.mpi python bindings, which can be obtained from
# http://mathema.tician.de/software/boostmpi and obviously an MPI-enabled Mantid build.
#
#############################################################################################

from MantidFramework import *
mtd.initialise()

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
    import boostmpi as mpi

    if mpi.world.size != len(runs)+1:
        print "This script must be run with " + str(len(runs)+1) + " MPI processes!"
        from sys import exit
        exit()

    if mpi.world.rank == len(runs):
        van = focus("PG3_1875_event.nxs", "PG3_1875", binning)
    else:
        name = "PG3_%d" % runs[mpi.world.rank]
        data = focus(name + "_event.nxs", name, binning)
        van = None

    BroadcastWorkspace(InputWorkspace=van,OutputWorkspace="Vanadium",BroadcasterRank=len(runs))

    if mpi.world.rank < len(runs):
        van = mtd["Vanadium"]
        data /= van
    else:
        data = None

    GatherWorkspaces(InputWorkspace=data, OutputWorkspace="Mn5Si3")

    if mpi.world.rank == 0:
        SaveNexus(InputWorkspace="Mn5Si3",Filename="mpi.nxs")

def loopmethod():
    van = focus("PG3_1875_event.nxs", "PG3_1875", binning)
    names = []
    for run in runs:
        name = "PG3_%d" % run
        data = focus(name + "_event.nxs", name, binning)
        data /= van
        names.append(str(data))

    for name in names[1:]:
            ConjoinWorkspaces(InputWorkspace1=names[0], InputWorkspace2=name,
                              CheckOverlapping=False)
    RenameWorkspace(InputWorkspace=names[0], OutputWorkspace="Mn5Si3")
    SaveNexus(InputWorkspace="Mn5Si3",Filename="nompi.nxs")

# Uncomment the method you want to run.
# On my machine, the mpi one takes ~9s, the single process method ~18s.
mpimethod()
#loopmethod()
