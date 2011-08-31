from MantidFramework import *
mtd.initialise()

back = 'NOM_1000_event.nxs'#'/SNS/NOM/2011_2_1B_SCI/1/1000/preNeXus/NOM_1000_neutron_event.dat' # 1GB
van = 'NOM_998_event.nxs'#Cheat for now (real file is huge) #van = '/SNS/users/pf9/NOM_989_event.nxs'#'/SNS/NOM/2011_2_1B_SCI/1/989/preNeXus/NOM_989_neutron_event.dat' # 73GB
#dia = '/SNS/users/pf9/NOM_990_event.nxs'#'/SNS/NOM/2011_2_1B_SCI/1/990/preNeXus/NOM_990_neutron_event.dat' # 65GB
sio2 = 'NOM_998_event.nxs'#'/NOM-DAS-FS/2011_2_1B_SCI/NOM_998/NOM_998_neutron_event.dat' # 5.6GB
#calib = 'NOM_calibrate_d739_2011_03_29.cal'
calib = 'NOM_2011_03_07.cal'
#mapping = '/SNS/NOM/2010_2_1B_CAL/calibrations/NOM_TS_2010_12_01.dat'


import os
import boostmpi as mpi

# Save typing
comm = mpi.world

if comm.size > 99:
    print "This script must be run with fewer than 99 MPI processes! (99 banks in NOMAD)"
    from sys import exit
    exit()

def focus(filename):
    # make a nice workspace name
    wksp = os.path.split(filename)[-1]
    wksp = '_'.join(wksp.split('_')[0:2]) + "-" + str(1+comm.rank)

    # Have a guard so that all processes don't hit the same file at the same time.
    # All processes except the root wait for a signal that the (rank-1) process has finished.
    if comm.rank > 0:
        comm.recv(comm.rank-1)
    # Need SingleBankPixelsOnly switched off or DiffractionFocussing fails
    LoadEventNexus(Filename=filename, OutputWorkspace=wksp,
                   BankName="bank"+str(1+comm.rank),
                   SingleBankPixelsOnly=False, CompressTolerance=.01)
    if comm.rank < comm.size-1:
        comm.isend(comm.rank+1)
    # TODO: Check for zero events here?
    NormaliseByCurrent(InputWorkspace=wksp, OutputWorkspace=wksp)
    AlignDetectors(InputWorkspace=wksp, OutputWorkspace=wksp,
                   CalibrationFile=calib)
    ConvertUnits(InputWorkspace=wksp, OutputWorkspace=wksp,
                 Target="MomentumTransfer")
    Rebin(InputWorkspace=wksp, OutputWorkspace=wksp, Params=(0.02,0.02,50))
    DiffractionFocussing(InputWorkspace=wksp, OutputWorkspace=wksp,
                         GroupingWorkspace="grouping", PreserveEvents=False)
    return mtd[wksp]
    

CreateGroupingWorkspace(InstrumentName='NOMAD', GroupNames='NOMAD', OutputWorkspace="grouping")
#CreateGroupingWorkspace(InstrumentName='NOMAD', OldCalFilename=calib, OutputWorkspace="grouping")
print back
back = focus(back)
print back
print sio2
sio2 = focus(sio2)
print sio2
print van
van = focus(van)
print van

sio2 -= back
van -= back
sio2 /= van

done = "Done " + str(1+comm.rank) + "!"
GatherWorkspaces(InputWorkspace=back, OutputWorkspace="nomad")
if comm.rank == 0:
    SumSpectra("nomad","nomad")
    SaveNexus(InputWorkspace="nomad",Filename="NOMAD.nxs")
print done

