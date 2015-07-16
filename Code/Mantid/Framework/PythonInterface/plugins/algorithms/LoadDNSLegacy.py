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


class LoadDNSLegacy(PythonAlgorithm):
    """
    Load the DNS Legacy data file to the mantid workspace
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
                             doc="Type of polarisation. Valid values: %s" % str(POLARISATIONS))
        return

    def PyExec(self):
        # Input
        filename = self.getPropertyValue("Filename")
        outws_name = self.getPropertyValue("OutputWorkspace")
        pol = self.getPropertyValue("Polarisation")

        # load data array from the given file
        data_array = np.loadtxt(filename)
        if not data_array.size:
            message = "File " + filename + " does not contain any data!"
            self.log().error(message)
            raise RuntimeError(message)
            return

        # load run information
        metadata = DNSdata()
        try:
            metadata.read_legacy(filename)
        except RuntimeError as err:
            message = "Error of loading of file " + filename + ": " + err
            self.log().error(message)
            raise RuntimeError(message)
            return

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
        api.RotateInstrumentComponent(outws,
                                      "bank0", X=0, Y=1, Z=0, Angle=metadata.deterota)
        # add sample log Ei and wavelength
        api.AddSampleLog(outws,
                         'Ei', LogText=str(metadata.incident_energy),
                         LogType='Number')
        api.AddSampleLog(outws,
                         'wavelength', LogText=str(metadata.wavelength),
                         LogType='Number')
        # add other sample logs
        api.AddSampleLog(outws, 'deterota',
                         LogText=str(metadata.deterota), LogType='Number')
        api.AddSampleLog(outws, 'mon_sum',
                         LogText=str(float(metadata.monitor_counts)), LogType='Number')
        api.AddSampleLog(outws, 'duration',
                         LogText=str(metadata.duration), LogType='Number')
        api.AddSampleLog(outws, 'huber',
                         LogText=str(metadata.huber), LogType='Number')
        api.AddSampleLog(outws, 'T1',
                         LogText=str(metadata.t1), LogType='Number')
        api.AddSampleLog(outws, 'T2',
                         LogText=str(metadata.t2), LogType='Number')
        api.AddSampleLog(outws, 'Tsp',
                         LogText=str(metadata.tsp), LogType='Number')
        # flipper
        api.AddSampleLog(outws, 'flipper_precession',
                         LogText=str(metadata.flipper_precession_current), LogType='Number')
        api.AddSampleLog(outws, 'flipper_z_compensation',
                         LogText=str(metadata.flipper_z_compensation_current), LogType='Number')
        flipper_status = 0.0    # flipper OFF
        if abs(metadata.flipper_precession_current) > sys.float_info.epsilon:
            flipper_status = 1.0    # flipper ON
        api.AddSampleLog(outws, 'flipper',
                         LogText=str(flipper_status), LogType='Number')
        # coil currents
        api.AddSampleLog(outws, 'C_a',
                         LogText=str(metadata.a_coil_current), LogType='Number')
        api.AddSampleLog(outws, 'C_b',
                         LogText=str(metadata.b_coil_current), LogType='Number')
        api.AddSampleLog(outws, 'C_c',
                         LogText=str(metadata.c_coil_current), LogType='Number')
        api.AddSampleLog(outws, 'C_z',
                         LogText=str(metadata.z_coil_current), LogType='Number')
        # type of polarisation
        api.AddSampleLog(outws, 'polarisation',
                         LogText=pol, LogType='String')
        # slits
        api.AddSampleLog(outws, 'slit_i_upper_blade_position',
                         LogText=str(metadata.slit_i_upper_blade_position), LogType='String')
        api.AddSampleLog(outws, 'slit_i_lower_blade_position',
                         LogText=str(metadata.slit_i_lower_blade_position), LogType='String')
        api.AddSampleLog(outws, 'slit_i_left_blade_position',
                         LogText=str(metadata.slit_i_left_blade_position), LogType='String')
        api.AddSampleLog(outws, 'slit_i_right_blade_position',
                         LogText=str(metadata.slit_i_right_blade_position), LogType='String')

        self.setProperty("OutputWorkspace", outws)
        self.log().debug('LoadDNSLegacy: data are loaded to the workspace ' + outws_name)

        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadDNSLegacy)
