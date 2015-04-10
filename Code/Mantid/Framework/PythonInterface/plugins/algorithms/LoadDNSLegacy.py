from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as api
import numpy as np

import os, sys

sys.path.insert(0, os.path.dirname(__file__))
from dnsdata import DNSdata
sys.path.pop(0)

POLARISATIONS = ['0', 'x', 'y', 'z', '-x', '-y', '-z']

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
        self.declareProperty("Polarisation", "0", \
                StringListValidator(POLARISATIONS), \
                doc="Type of polarisation. Valid values: %s" % str(POLARISATIONS))
        return


    def PyExec(self):
        # Input
        filename = self.getPropertyValue("Filename")
        outws = self.getPropertyValue("OutputWorkspace")
        pol = self.getPropertyValue("Polarisation")

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
        if metadata.start_time and metadata.end_time:
            run.setStartAndEndTime(DateAndTime(metadata.start_time), \
                    DateAndTime(metadata.end_time))
        # add name of file as a run title
        fname = os.path.splitext(os.path.split(filename)[1])[0]
        run.addProperty('run_title', fname, True)
        #run.addProperty('dur_secs', str(metadata.duration), True)

        # rotate the detector bank to the proper position
        api.RotateInstrumentComponent(__temporary_workspace__, \
                "bank0", X=0, Y=1, Z=0, Angle=metadata.deterota)
        # add sample log Ei and wavelength
        api.AddSampleLog(__temporary_workspace__, \
                'Ei', LogText=str(metadata.incident_energy), \
                LogType='Number')
        api.AddSampleLog(__temporary_workspace__, \
                'wavelength', LogText=str(metadata.wavelength), \
                LogType='Number')
        # add other sample logs
        api.AddSampleLog(__temporary_workspace__, 'deterota', \
                LogText=str(metadata.deterota), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'mon_sum', \
                LogText=str(metadata.monitor_counts), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'duration', \
                LogText=str(metadata.duration), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'huber', \
                LogText=str(metadata.huber), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'T1', \
                LogText=str(metadata.t1), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'T2', \
                LogText=str(metadata.t2), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'Tsp', \
                LogText=str(metadata.tsp), LogType='Number')
        # flipper
        api.AddSampleLog(__temporary_workspace__, 'flipper_precession', \
                LogText=str(metadata.flipper_precession_current), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'flipper_z_compensation', \
                LogText=str(metadata.flipper_z_compensation_current), LogType='Number')
        flipper_status = 'OFF'
        if abs(metadata.flipper_precession_current) > sys.float_info.epsilon:
            flipper_status = 'ON'
        api.AddSampleLog(__temporary_workspace__, 'flipper', \
                LogText=flipper_status, LogType='String')
        # coil currents
        api.AddSampleLog(__temporary_workspace__, 'C_a', \
                LogText=str(metadata.a_coil_current), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'C_b', \
                LogText=str(metadata.b_coil_current), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'C_c', \
                LogText=str(metadata.c_coil_current), LogType='Number')
        api.AddSampleLog(__temporary_workspace__, 'C_z', \
                LogText=str(metadata.z_coil_current), LogType='Number')
        # type of polarisation
        api.AddSampleLog(__temporary_workspace__, 'polarisation', \
                LogText=pol, LogType='String')

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
