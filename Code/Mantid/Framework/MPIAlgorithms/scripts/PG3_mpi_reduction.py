from mantid.simpleapi import *

import os
from mpi4py import MPI

# Save typing
comm = MPI.COMM_WORLD

SNSPowderReduction(Instrument='PG3', RunNumber='2538', Extension='_event.nxs', PreserveEvents=False, CalibrationFile='PG3_calibrate_d2538_2012_02_21.cal', Binning='0.2,-0.0004,10.',
BinInDspace=True, StripVanadiumPeaks=True, FilterBadPulses=True, NormalizeByCurrent=False, SaveAs='gsas', OutputDirectory='.', FinalDataUnits='dSpacing', MaxChunkSize = 0.6)
if comm.rank == 0:
    os.system("mv PG3_2538.gsa mpi8_histo.gsa")
SNSPowderReduction(Instrument='PG3', RunNumber='2538', Extension='_event.nxs', PreserveEvents=True, CalibrationFile='PG3_calibrate_d2538_2012_02_21.cal', Binning='0.2,-0.0004,10.',
BinInDspace=True, StripVanadiumPeaks=True, FilterBadPulses=True, NormalizeByCurrent=False, SaveAs='gsas', OutputDirectory='.', FinalDataUnits='dSpacing', MaxChunkSize = 0.6)
if comm.rank == 0:
    os.system("mv PG3_2538.gsa mpi8_event.gsa")

done = "Done " + str(1+comm.rank) + "!"
print done

