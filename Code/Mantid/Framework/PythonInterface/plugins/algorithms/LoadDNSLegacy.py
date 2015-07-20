#pylint: disable=no-init
import mantid.simpleapi as api
import numpy as np
import os
import sys
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, \
    FileProperty, FileAction
from mantid.kernel import Direction, StringListValidator, DateAndTime

sys.path.insert(0, os.path.dirname(__file__))
from dnsdata import DNSdata
sys.path.pop(0)

POLARISATIONS = ['0', 'x', 'y', 'z', '-x', '-y', '-z']
NORMALIZATIONS = ['duration', 'monitor']


class LoadDNSLegacy(PythonAlgorithm):
    """
    Load the DNS Legacy data file to the matrix workspace
    Monitor/duration data are loaded to the separate workspace
    """
    def category(self):
        """
        Returns category
        """
        return 'PythonAlgorithms\\MLZ\\DNS\\DataHandling'

    def name(self):
        """
        Returns name
        """
        return "LoadDNSLegacy"

    def summary(self):
        return "Load the DNS Legacy data file to the mantid workspace."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "",
                                          FileAction.Load, ['.d_dat']),
                             "Name of DNS experimental data file.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace",
                                               "", direction=Direction.Output),
                             doc="Name of the workspace to store the experimental data.")
        self.declareProperty("Polarisation", "0",
                             StringListValidator(POLARISATIONS),
                             doc="Type of polarisation.")
        self.declareProperty("Normalization", "duration",
                             StringListValidator(NORMALIZATIONS),
                             doc="Type of data for normalization.")
        return

    def PyExec(self):
        # Input
        filename = self.getPropertyValue("Filename")
        outws_name = self.getPropertyValue("OutputWorkspace")
        monws_name = outws_name + '_NORM'
        pol = self.getPropertyValue("Polarisation")
        norm = self.getPropertyValue("Normalization")

        # load data array from the given file
        data_array = np.loadtxt(filename)
        if not data_array.size:
            message = "File " + filename + " does not contain any data!"
            self.log().error(message)
            raise RuntimeError(message)

        # load run information
        metadata = DNSdata()
        try:
            metadata.read_legacy(filename)
        except RuntimeError as err:
            message = "Error of loading of file " + filename + ": " + err
            self.log().error(message)
            raise RuntimeError(message)

        ndet = 24
        # this needed to be able to use ConvertToMD
        dataX = np.zeros(2*ndet)
        dataX.fill(metadata.wavelength + 0.00001)
        dataX[::2] -= 0.000002
        dataY = data_array[0:ndet, 1:]
        dataE = np.sqrt(dataY)
        # create workspace
        api.CreateWorkspace(OutputWorkspace=outws_name, DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        outws = api.mtd[outws_name]
        api.LoadInstrument(outws, InstrumentName='DNS')

        run = outws.mutableRun()
        if metadata.start_time and metadata.end_time:
            run.setStartAndEndTime(DateAndTime(metadata.start_time),
                                   DateAndTime(metadata.end_time))
        # add name of file as a run title
        fname = os.path.splitext(os.path.split(filename)[1])[0]
        run.addProperty('run_title', fname, True)

        # rotate the detector bank to the proper position
        api.RotateInstrumentComponent(outws, "bank0", X=0, Y=1, Z=0, Angle=metadata.deterota)
        # add sample log Ei and wavelength
        api.AddSampleLog(outws, LogName='Ei', LogText=str(metadata.incident_energy),
                         LogType='Number', LogUnit='meV')
        api.AddSampleLog(outws, LogName='wavelength', LogText=str(metadata.wavelength),
                         LogType='Number', LogUnit='Angstrom')
        # add other sample logs
        api.AddSampleLog(outws, LogName='deterota', LogText=str(metadata.deterota),
                         LogType='Number', LogUnit='Degrees')
        api.AddSampleLog(outws, 'mon_sum',
                         LogText=str(float(metadata.monitor_counts)), LogType='Number')
        api.AddSampleLog(outws, LogName='duration', LogText=str(metadata.duration),
                         LogType='Number', LogUnit='Seconds')
        api.AddSampleLog(outws, LogName='huber', LogText=str(metadata.huber),
                         LogType='Number', LogUnit='Degrees')
        api.AddSampleLog(outws, LogName='omega', LogText=str(metadata.huber - metadata.deterota),
                         LogType='Number', LogUnit='Degrees')
        api.AddSampleLog(outws, LogName='T1', LogText=str(metadata.t1),
                         LogType='Number', LogUnit='K')
        api.AddSampleLog(outws, LogName='T2', LogText=str(metadata.t2),
                         LogType='Number', LogUnit='K')
        api.AddSampleLog(outws, LogName='Tsp', LogText=str(metadata.tsp),
                         LogType='Number', LogUnit='K')
        # flipper
        api.AddSampleLog(outws, LogName='flipper_precession',
                         LogText=str(metadata.flipper_precession_current),
                         LogType='Number', LogUnit='A')
        api.AddSampleLog(outws, LogName='flipper_z_compensation',
                         LogText=str(metadata.flipper_z_compensation_current),
                         LogType='Number', LogUnit='A')
        flipper_status = 'OFF'    # flipper OFF
        if abs(metadata.flipper_precession_current) > sys.float_info.epsilon:
            flipper_status = 'ON'    # flipper ON
        api.AddSampleLog(outws, LogName='flipper',
                         LogText=flipper_status, LogType='String')
        # coil currents
        api.AddSampleLog(outws, LogName='C_a', LogText=str(metadata.a_coil_current),
                         LogType='Number', LogUnit='A')
        api.AddSampleLog(outws, LogName='C_b', LogText=str(metadata.b_coil_current),
                         LogType='Number', LogUnit='A')
        api.AddSampleLog(outws, LogName='C_c', LogText=str(metadata.c_coil_current),
                         LogType='Number', LogUnit='A')
        api.AddSampleLog(outws, LogName='C_z', LogText=str(metadata.z_coil_current),
                         LogType='Number', LogUnit='A')
        # type of polarisation
        api.AddSampleLog(outws, 'polarisation',
                         LogText=pol, LogType='String')
        # slits
        api.AddSampleLog(outws, LogName='slit_i_upper_blade_position',
                         LogText=str(metadata.slit_i_upper_blade_position),
                         LogType='Number', LogUnit='mm')
        api.AddSampleLog(outws, LogName='slit_i_lower_blade_position',
                         LogText=str(metadata.slit_i_lower_blade_position),
                         LogType='Number', LogUnit='mm')
        api.AddSampleLog(outws, LogName='slit_i_left_blade_position',
                         LogText=str(metadata.slit_i_left_blade_position),
                         LogType='Number', LogUnit='mm')
        api.AddSampleLog(outws, 'slit_i_right_blade_position',
                         LogText=str(metadata.slit_i_right_blade_position),
                         LogType='Number', LogUnit='mm')

        # create workspace with normalization data (monitor or duration)
        if norm == 'duration':
            dataY.fill(metadata.duration)
            dataE.fill(0.001)
        else:
            dataY.fill(metadata.monitor_counts)
            dataE = np.sqrt(dataY)
        api.CreateWorkspace(OutputWorkspace=monws_name, DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        monws = api.mtd[monws_name]
        api.LoadInstrument(monws, InstrumentName='DNS')
        api.CopyLogs(InputWorkspace=outws_name, OutputWorkspace=monws_name, MergeStrategy='MergeReplaceExisting')

        self.setProperty("OutputWorkspace", outws)
        self.log().debug('LoadDNSLegacy: data are loaded to the workspace ' + outws_name)

        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadDNSLegacy)
