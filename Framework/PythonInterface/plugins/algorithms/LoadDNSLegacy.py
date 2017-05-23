from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as api
import numpy as np
import os
import sys
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, \
    FileProperty, FileAction
from mantid.kernel import Direction, StringListValidator, DateAndTime

from dnsdata import DNSdata


class LoadDNSLegacy(PythonAlgorithm):
    """
    Load the DNS Legacy data file to the matrix workspace
    Monitor/duration data are loaded to the separate workspace
    """

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.tolerance = 1e-2

    def category(self):
        """
        Returns category
        """
        return 'Workflow\\MLZ\\DNS;DataHandling\\Text'

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

        self.declareProperty(FileProperty("CoilCurrentsTable", "",
                                          FileAction.Load, ['.txt']),
                             "Name of file containing table of coil currents and polarisations.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace",
                                               "", direction=Direction.Output),
                             doc="Name of the workspace to store the experimental data.")
        normalizations = ['duration', 'monitor', 'no']
        self.declareProperty("Normalization", "duration", StringListValidator(normalizations),
                             doc="Kind of data normalization.")
        return

    def get_polarisation_table(self):
        # load polarisation table
        poltable_name = self.getPropertyValue("CoilCurrentsTable")
        try:
            currents = np.genfromtxt(poltable_name, names=True, dtype=None)
        except ValueError as err:
            raise RuntimeError("Invalid coil currents table: " + str(err))
        poltable = []
        colnames = currents.dtype.names
        poltable = [dict(list(zip(colnames, cur))) for cur in currents]
        self.log().debug("Loaded polarisation table:\n" + str(poltable))
        return poltable

    def currents_match(self, dict1, dict2):
        keys = ['C_a', 'C_b', 'C_c', 'C_z']
        for key in keys:
            if np.fabs(dict1[key] - dict2[key]) > self.tolerance:
                return False
        return True

    def get_polarisation(self, metadata, poltable):
        pol = []
        coilcurrents = {'C_a': metadata.a_coil_current, 'C_b': metadata.b_coil_current,
                        'C_c': metadata.c_coil_current, 'C_z': metadata.z_coil_current}
        self.log().debug("Coil currents are " + str(coilcurrents))
        for row in poltable:
            if self.currents_match(row, coilcurrents):
                return [row['polarisation'], row['comment']]

        return pol

    def PyExec(self):
        # Input
        filename = self.getPropertyValue("Filename")
        outws_name = self.getPropertyValue("OutputWorkspace")
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
            message = "Error of loading of file " + filename + ": " + str(err)
            self.log().error(message)
            raise RuntimeError(message)

        # load polarisation table and determine polarisation
        poltable = self.get_polarisation_table()
        pol = self.get_polarisation(metadata, poltable)
        if not pol:
            pol = ['0', 'undefined']
            self.log().warning("Failed to determine polarisation for " + filename +
                               ". Values have been set to undefined.")
        ndet = 24
        # this needed to be able to use ConvertToMD
        dataX = np.zeros(2*ndet)
        dataX.fill(metadata.wavelength + 0.00001)
        dataX[::2] -= 0.000002
        # data normalization
        factor = 1.0
        yunit = "Counts"
        ylabel = "Intensity"
        if norm == 'duration':
            factor = metadata.duration
            yunit = "Counts/s"
            ylabel = "Intensity normalized to duration"
            if factor <= 0:
                raise RuntimeError("Duration is invalid for file " + filename + ". Cannot normalize.")
        if norm == 'monitor':
            factor = metadata.monitor_counts
            yunit = "Counts/monitor"
            ylabel = "Intensity normalized to monitor"
            if factor <= 0:
                raise RuntimeError("Monitor counts are invalid for file " + filename + ". Cannot normalize.")
        # set values for dataY and dataE
        dataY = data_array[0:ndet, 1:]/factor
        dataE = np.sqrt(data_array[0:ndet, 1:])/factor
        # create workspace
        api.CreateWorkspace(OutputWorkspace=outws_name, DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        outws = api.AnalysisDataService.retrieve(outws_name)
        api.LoadInstrument(outws, InstrumentName='DNS', RewriteSpectraMap=True)

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
        api.AddSampleLog(outws, LogName='T1', LogText=str(metadata.temp1),
                         LogType='Number', LogUnit='K')
        api.AddSampleLog(outws, LogName='T2', LogText=str(metadata.temp2),
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
        api.AddSampleLog(outws, 'polarisation', LogText=pol[0], LogType='String')
        api.AddSampleLog(outws, 'polarisation_comment', LogText=str(pol[1]), LogType='String')
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
        # data normalization

        # add information whether the data are normalized (duration/monitor/no):
        api.AddSampleLog(outws, LogName='normalized', LogText=norm, LogType='String')

        outws.setYUnit(yunit)
        outws.setYUnitLabel(ylabel)

        self.setProperty("OutputWorkspace", outws)
        self.log().debug('LoadDNSLegacy: data are loaded to the workspace ' + outws_name)

        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadDNSLegacy)
