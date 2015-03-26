from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as api
import numpy as np

from dnsdata import DNSdata

class LoadDNSLegacy(PythonAlgorithm):
    """
    Load the DNS Legacy data file to the mantid workspace
    """
    def category(self):
        """
        Returns categore
        """
        return 'DataHandling'

    def name(self):
        """
        Returns name
        """
        return "LoadDNSLegacy"

    def summary(self):
        return "Load the DNS Legacy data file to the mantid workspace."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "",  \
                FileAction.Load, ['.d_dat']), \
                "Name of DNS experimental data file.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", \
                "", direction=Direction.Output), \
                doc="Name of the workspace to store the experimental data.")

        return


    def PyExec(self):
        # Input
        filename = self.getPropertyValue("Filename")
        outws = self.getPropertyValue("OutputWorkspace")

        # load data array from the given file
        data_array = np.loadtxt(filename)
        ndet = 24
        dataX = np.zeros(ndet)
        dataY = data_array[0:ndet, 1:]
        dataE = np.sqrt(dataY)
        # create workspace
        __temporary_workspace__ = api.CreateWorkspace(DataX=dataX, \
                DataY=dataY, DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        api.LoadInstrument(__temporary_workspace__, InstrumentName='DNS')

        # load run information
        metadata = DNSdata()
        metadata.read_legacy(filename)
        run = __temporary_workspace__.mutableRun()
        run.setStartAndEndTime(DateAndTime(metadata.start_time), \
                DateAndTime(metadata.end_time))

        # rotate the detector bank to the proper position
        api.RotateInstrumentComponent(__temporary_workspace__, \
                "bank0", X=0, Y=1, Z=0, Angle=metadata.deterota)
        # add sample log Ei
        energy = get_energy(metadata.wavelength)
        api.AddSampleLog(__temporary_workspace__, \
                'Ei', LogText=str(energy), LogType='Number')
        # add other sample logs
        api.AddSampleLog(__temporary_workspace__, 'deterota', \
                LogText=str(metadata.deterota), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'huber', \
                LogText=str(metadata.huber), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'T1', \
                LogText=str(metadata.t1), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'T2', \
                LogText=str(metadata.t2), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'Tsp', \
                LogText=str(metadata.tsp), LogType='Number')

        self.setProperty("OutputWorkspace", __temporary_workspace__)
        self.log().debug('LoadDNSLegacy: OK')
        api.DeleteWorkspace(__temporary_workspace__)

        return


def get_energy(wavelength):
    """
    Calculates neutron energy in eV from the given wavelength in Angstrom
    """
    return  1e-3*81.73 / wavelength**2

# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadDNSLegacy)
