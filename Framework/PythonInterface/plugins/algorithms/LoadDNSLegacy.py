# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as api
import numpy as np
from scipy.constants import m_n, h, physical_constants
import os
import sys
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, \
    FileProperty, FileAction
from mantid.kernel import Direction, StringListValidator, DateAndTime, IntBoundedValidator, FloatBoundedValidator

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
        self.instrument = None

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
                                          FileAction.OptionalLoad, ['.txt']),
                             "Name of file containing table of coil currents and polarisations.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace",
                                               "", direction=Direction.Output),
                             doc="Name of the workspace to store the experimental data.")
        normalizations = ['duration', 'monitor', 'no']
        self.declareProperty("Normalization", "duration", StringListValidator(normalizations),
                             doc="Kind of data normalization.")

        self.declareProperty(name="ElasticChannel",defaultValue=0,validator=IntBoundedValidator(lower=0),
                             doc="Time channel number where elastic peak is observed. Only for TOF data.")

        self.declareProperty(name="Wavelength",defaultValue=0.0,validator=FloatBoundedValidator(lower=0.0),
                             doc="Wavelength in nm. If 0 will be read from data file.")
        return

    def get_polarisation_table(self):
        # load polarisation table
        poltable = []
        poltable_name = self.getPropertyValue("CoilCurrentsTable")
        if not poltable_name:
            # read the table from IDF
            for p in ['x', 'y', 'z']:
                currents = self.instrument.getStringParameter("{}_currents".format(p))[0].split(';')
                for cur in currents:
                    row = {'polarisation': p, 'comment': '7'}
                    row['C_a'], row['C_b'], row['C_c'], row['C_z'] = [float(c) for c in cur.split(',')]
                    poltable.append(row)
            self.log().debug("Loaded polarisation table:\n" + str(poltable))
            return poltable
        try:
            currents = np.genfromtxt(poltable_name, names=True, dtype='U2,U2,f8,f8,f8,f8')
            self.log().debug("Coil currents are: " + str(currents))
        except ValueError as err:
            raise RuntimeError("Invalid coil currents table: " + str(err))
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
        wavelength = self.getProperty("Wavelength").value

        # load data array from the given file
        data_array = np.loadtxt(filename)
        if not data_array.size:
            message = "File " + filename + " does not contain any data!"
            self.log().error(message)
            raise RuntimeError(message)
        # sample logs
        logs = {"names": [], "values": [], "units": []}

        # load run information
        metadata = DNSdata()
        try:
            metadata.read_legacy(filename)
        except RuntimeError as err:
            message = "Error of loading of file " + filename + ": " + str(err)
            self.log().error(message)
            raise RuntimeError(message)

        if wavelength < 0.01:
            wavelength = metadata.wavelength

        # calculate incident energy, since given in the data file is wrong
        velocity = h/(m_n*wavelength*1e-10)   # m/s
        incident_energy = 0.5e+03*m_n*velocity*velocity/physical_constants['electron volt'][0]  # meV

        tmp = api.LoadEmptyInstrument(InstrumentName='DNS')
        self.instrument = tmp.getInstrument()
        api.DeleteWorkspace(tmp)

        # load polarisation table and determine polarisation
        poltable = self.get_polarisation_table()
        pol = self.get_polarisation(metadata, poltable)
        if not pol:
            pol = ['0', 'undefined']
            self.log().warning("Failed to determine polarisation for " + filename +
                               ". Values have been set to undefined.")
        ndet = 24
        unitX="Wavelength"
        arr = data_array[0:ndet, 1:]
        if metadata.tof_channel_number < 2:
            dataX = np.zeros(2*ndet)
            dataX.fill(wavelength + 0.00001)
            dataX[::2] -= 0.000002
        else:
            unitX="TOF"

            # get instrument parameters
            l1 = np.linalg.norm(self.instrument.getSample().getPos() - self.instrument.getSource().getPos())
            l2 = float(self.instrument.getStringParameter("l2")[0])
            self.log().notice("L1 = {} m".format(l1))
            self.log().notice("L2 = {} m".format(l2))
            dt_factor = float(self.instrument.getStringParameter("channel_width_factor")[0])

            # channel width
            dt = metadata.tof_channel_width*dt_factor
            # calculate tof1
            tof1 = 1e+06*l1/velocity        # microseconds
            self.log().debug("TOF1 = {} microseconds".format(tof1))
            self.log().debug("Delay time = {} microsecond".format(metadata.tof_delay_time))
            tof2_elastic = 1e+06*l2/velocity
            self.log().debug("TOF2 Elastic = {} microseconds".format(tof2_elastic))
            epp_geom = int(tof2_elastic/dt)

            epp_user = self.getProperty("ElasticChannel").value

            # for comissioning period EPP in the data file is not relevant
            in_comissioning = self.instrument.getStringParameter("tof_comissioning")[0]
            if (epp_user < 1) and (in_comissioning == 'no') and  metadata.tof_elastic_channel:
                epp_user = metadata.tof_elastic_channel

            # shift channels to keep elastic in the right position
            # required, since zero time channel is not calibrated
            if epp_user > 0:
                arr = np.roll(arr, epp_geom - epp_user, 1)

            # create dataX array
            x0 = tof1 + metadata.tof_delay_time
            dataX = np.linspace(x0, x0+metadata.tof_channel_number*dt, metadata.tof_channel_number+1)

            # sample logs
            logs["names"].extend(["channel_width", "TOF1", "delay_time", "tof_channels"])
            logs["values"].extend([dt, tof1, metadata.tof_delay_time, metadata.tof_channel_number])
            logs["units"].extend(["microseconds", "microseconds", "microseconds", ""])
            if epp_user:
                logs["names"].append("EPP")
                logs["values"].append(epp_user)
                logs["units"].append("")
            if metadata.chopper_rotation_speed:
                logs["names"].append("chopper_speed")
                logs["values"].append(metadata.chopper_rotation_speed)
                logs["units"].append("Hz")
            if metadata.chopper_slits:
                logs["names"].append("chopper_slits")
                logs["values"].append(metadata.chopper_slits)
                logs["units"].append("")

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
        dataY = arr/factor
        dataE = np.sqrt(arr)/factor

        # create workspace
        api.CreateWorkspace(OutputWorkspace=outws_name, DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX=unitX)
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
        logs["names"].extend(["Ei", "wavelength"])
        logs["values"].extend([incident_energy, wavelength])
        logs["units"].extend(["meV", "Angstrom"])

        # add other sample logs
        logs["names"].extend(["deterota", "mon_sum", "duration", "huber", "omega", "T1", "T2", "Tsp"])
        logs["values"].extend([metadata.deterota, float(metadata.monitor_counts), metadata.duration,
                               metadata.huber, metadata.huber - metadata.deterota,
                               metadata.temp1, metadata.temp2, metadata.tsp])
        logs["units"].extend(["Degrees", "Counts", "Seconds", "Degrees", "Degrees", "K", "K", "K"])

        # flipper, coil currents and polarisation
        flipper_status = 'OFF'    # flipper OFF
        if abs(metadata.flipper_precession_current) > sys.float_info.epsilon:
            flipper_status = 'ON'    # flipper ON
        logs["names"].extend(["flipper_precession", "flipper_z_compensation", "flipper",
                              "C_a", "C_b", "C_c", "C_z", "polarisation", "polarisation_comment"])
        logs["values"].extend([metadata.flipper_precession_current,
                               metadata.flipper_z_compensation_current, flipper_status,
                               metadata.a_coil_current, metadata.b_coil_current,
                               metadata.c_coil_current, metadata.z_coil_current,
                               str(pol[0]), str(pol[1])])
        logs["units"].extend(["A", "A", "", "A", "A", "A", "A", "", ""])

        # slits
        logs["names"].extend(["slit_i_upper_blade_position", "slit_i_lower_blade_position",
                              "slit_i_left_blade_position", "slit_i_right_blade_position"])
        logs["values"].extend([metadata.slit_i_upper_blade_position, metadata.slit_i_lower_blade_position,
                               metadata.slit_i_left_blade_position, metadata.slit_i_right_blade_position])
        logs["units"].extend(["mm", "mm", "mm", "mm"])

        # add information whether the data are normalized (duration/monitor/no):
        api.AddSampleLog(outws, LogName='normalized', LogText=norm, LogType='String')
        api.AddSampleLogMultiple(outws, LogNames=logs["names"], LogValues=logs["values"], LogUnits=logs["units"])

        outws.setYUnit(yunit)
        outws.setYUnitLabel(ylabel)

        self.setProperty("OutputWorkspace", outws)
        self.log().debug('LoadDNSLegacy: data are loaded to the workspace ' + outws_name)

        return


# Register algorithm with Mantid
AlgorithmFactory.subscribe(LoadDNSLegacy)
