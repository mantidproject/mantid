# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from mantid import FileFinder
from mantid.api import mtd, AlgorithmFactory, AlgorithmManager, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, IntArrayBoundedValidator, IntArrayProperty, Logger
from mantid.simpleapi import CreateWorkspace, FilterByLogValue, Load, LoadInstrument

import math
import numpy
import sys
import os
import json


class USANSReduction(PythonAlgorithm):
    wl_list = None
    data_files = None
    total_points = None
    q_output = None
    iq_output = None
    iq_err_output = None

    def category(self):
        return "SANS"

    def seeAlso(self):
        return ["USANSSimulation"]

    def name(self):
        return "USANSReduction"

    def summary(self):
        return "Perform USANS data reduction"

    def PyInit(self):
        arrvalidator = IntArrayBoundedValidator(lower=0)
        self.declareProperty(
            IntArrayProperty("RunNumbers", values=[0], validator=arrvalidator, direction=Direction.Input), "Runs to reduce"
        )
        self.declareProperty("EmptyRun", "", "Run number for the empty run")

        # TODO: Mask workspace

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")

    # pylint: disable= too-few-public-methods
    class DataFile(object):
        # pylint: disable= too-many-arguments
        def __init__(self, workspace, monitor, empty, empty_monitor, is_scan=False, max_index=1):
            self.workspace = workspace
            self.monitor = monitor
            self.empty = empty
            self.empty_monitor = empty_monitor
            self.is_scan = is_scan
            self.max_index = max_index

    def _find_monitors(self, run):
        """
        Find a monitor file for testing purposes.
        @param run: run number
        """
        f_list = FileFinder.findRuns("USANS_%s" % run)
        if len(f_list) > 0:
            root, ext = os.path.splitext(f_list[0])
            return "%s_monitors%s" % (root, ext)
        else:
            Logger("USANSReduction").error("Could not find monitors for run %s" % run)
        return None

    def _load_data(self):
        """
        Load data and go through each file to determine how many points
        will have to be dealt with.
        """
        # Load the empty run
        empty_run = self.getProperty("EmptyRun").value
        Load(Filename="USANS_%s" % empty_run, LoadMonitors=True, OutputWorkspace="__empty")
        # A simple Load doesn't load the instrument properties correctly with our test file
        # Reload the instrument for now
        LoadInstrument(Workspace="__empty", InstrumentName="USANS", RewriteSpectraMap=False)

        # For testing, we may have to load the monitors by hand
        if not mtd.doesExist("__empty_monitors"):
            Load(Filename=self._find_monitors(empty_run), OutputWorkspace="__empty_monitors")

        # Get the wavelength peak positions
        wl_cfg_str = mtd["__empty"].getInstrument().getStringParameter("wavelength_config")[0]
        self.wl_list = json.loads(wl_cfg_str)

        # Get the runs to reduce
        run_list = self.getProperty("RunNumbers").value

        # Total number of measurements per wavelength peak
        total_points = 0
        # Load all files so we can determine how many points we have
        self.data_files = []
        for item in run_list:
            ws_name = "__sample_%s" % item
            Load(Filename="USANS_%s" % item, LoadMonitors=True, OutputWorkspace=ws_name)

            # For testing, we may have to load the monitors by hand
            if not mtd.doesExist(ws_name + "_monitors"):
                Load(Filename=self._find_monitors(empty_run), OutputWorkspace=ws_name + "_monitors")

            # Determine whether we are putting together multiple files or whether
            # we will be looking for scan_index markers.
            is_scan = False
            max_index = 1
            if mtd[ws_name].getRun().hasProperty("scan_index"):
                scan_index = mtd[ws_name].getRun().getProperty("scan_index").value
                if len(scan_index) > 0:
                    _max_index = scan_index.getStatistics().maximum
                    if _max_index > 0:
                        max_index = _max_index
                        is_scan = True

            # Append the info for when we do the reduction
            self.data_files.append(
                self.DataFile(
                    workspace=ws_name,
                    monitor=ws_name + "_monitors",
                    empty="__empty",
                    empty_monitor="__empty_monitors",
                    is_scan=is_scan,
                    max_index=max_index,
                )
            )
            total_points += max_index

        return total_points

    def _process_data_file(self, file_info, index_offset):
        # Go through each point
        for point in range(file_info.max_index):
            # If we are in a scan, select the current scan point
            if file_info.is_scan:
                ws = FilterByLogValue(
                    InputWorkspace=mtd[file_info.workspace],
                    LogName="scan_index",
                    MinimumValue=point,
                    MaximumValue=point,
                    LogBoundary="Left",
                )
            else:
                ws = mtd[file_info.workspace]

            # Get the two-theta value for this point
            if ws.getRun().getProperty("two_theta").type == "number":
                two_theta = ws.getRun().getProperty("two_theta").value
            else:
                two_theta = ws.getRun().getTimeAveragedValue("two_theta")

            # Loop through the wavelength peaks for this point
            for i_wl in range(len(self.wl_list)):
                wl = self.wl_list[i_wl]["wavelength"]
                # Note: TOF value is given by tof = 30.0/0.0039560*wl
                q = 6.28 * math.sin(two_theta) / wl

                # Get I(q) for each wavelength peak
                i_q = self._get_intensity(
                    mtd[file_info.workspace],
                    mtd[file_info.empty],
                    mtd[file_info.monitor],
                    mtd[file_info.empty_monitor],
                    tof_min=self.wl_list[i_wl]["t_min"],
                    tof_max=self.wl_list[i_wl]["t_max"],
                )
                # Store the reduced data
                try:
                    self.q_output[i_wl][point + index_offset] = q
                    self.iq_output[i_wl][point + index_offset] = i_q.dataY(0)[0]
                    self.iq_err_output[i_wl][point + index_offset] = i_q.dataE(0)[0]
                except:
                    Logger("USANSReduction").error(
                        "Exception caught for " + "%s on peak %s, point %s. Offset=%s" % (file_info.workspace, i_wl, point, index_offset)
                    )
                    Logger("USANSReduction").error(
                        "Array: " + "%s x %s    Data: %s" % (len(self.wl_list), self.total_points, file_info.max_index)
                    )
                    Logger("USANSReduction").error(sys.exc_info()[1])
        return file_info.max_index

    def PyExec(self):
        # Placeholder for the data file information
        self.data_files = []

        # Total number of measurements per wavelength peak
        self.total_points = self._load_data()

        # Create an array to store the I(q) points
        n_wl = len(self.wl_list)
        Logger("USANSReduction").notice("USANS reduction for %g peaks with %g point(s) each" % (n_wl, self.total_points))
        self.q_output = numpy.zeros(shape=(n_wl, self.total_points))
        self.iq_output = numpy.zeros(shape=(n_wl, self.total_points))
        self.iq_err_output = numpy.zeros(shape=(n_wl, self.total_points))

        index_offset = 0
        for item in self.data_files:
            index_offset += self._process_data_file(item, index_offset)

        # Create a workspace for each peak
        self._aggregate()

    def _aggregate(self):
        """
        Create a workspace for each peak
        #TODO: stitch the data instead of just dumping them in a workspace
        """
        x_all = []
        y_all = []
        e_all = []

        def compare(p1, p2):
            if p2[0] == p1[0]:
                return 0
            return -1 if p2[0] > p1[0] else 1

        for i_wl in range(len(self.wl_list)):
            x_all.extend(self.q_output[i_wl])
            y_all.extend(self.iq_output[i_wl])
            e_all.extend(self.iq_err_output[i_wl])
            x = self.q_output[i_wl]
            y = self.iq_output[i_wl]
            e = self.iq_err_output[i_wl]

            # Sort the I(q) point just in case we got them in the wrong order
            zipped = list(zip(x, y, e))
            combined = sorted(zipped, compare)
            x, y, e = list(zip(*combined))

            wl = self.wl_list[i_wl]["wavelength"]
            CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, UnitX="MomentumTransfer", OutputWorkspace="iq_%1.2f" % wl)

        # Sort the I(q) point just in case we got them in the wrong order
        zipped = list(zip(x_all, y_all, e_all))
        combined = sorted(zipped, compare)
        x, y, e = list(zip(*combined))

        # Create the combined output workspace
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        out_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, UnitX="MomentumTransfer", OutputWorkspace=output_ws_name)
        self.setProperty("OutputWorkspace", out_ws)

    # pylint: disable=too-many-arguments
    def _get_intensity(self, sample, empty, sample_monitor, empty_monitor, tof_min, tof_max):
        # Number of pixels we are dealing with
        nspecs = sample.getNumberHistograms()

        # Apply mask

        # Get the normalized empty run counts in the transmission detector
        __empty_summed = _execute(
            "SumSpectra",
            InputWorkspace=str(empty),
            StartWorkspaceIndex=nspecs / 2,
            EndWorkspaceIndex=nspecs - 1,
            OutputWorkspace="__empty_summed",
        )
        __point = _execute("CropWorkspace", InputWorkspace=__empty_summed, XMin=tof_min, XMax=tof_max, OutputWorkspace="__point")
        __empty_count = _execute("Integration", InputWorkspace=__point, OutputWorkspace="__empty_count")
        __point = _execute("CropWorkspace", InputWorkspace=str(empty_monitor), XMin=tof_min, XMax=tof_max, OutputWorkspace="__point")
        __empty_monitor_count = _execute("Integration", InputWorkspace=__point, OutputWorkspace="__empty_monitor_count")

        __normalized_empty = _execute(
            "Divide", LHSWorkspace=__empty_count, RHSWorkspace=__empty_monitor_count, OutputWorkspace="__normalized_empty"
        )

        # Get the normalized sample counts in the transmission detector
        __trans_summed = _execute(
            "SumSpectra",
            InputWorkspace=sample,
            StartWorkspaceIndex=nspecs / 2,
            EndWorkspaceIndex=nspecs - 1,
            OutputWorkspace="__trans_summed",
        )
        __point = _execute("CropWorkspace", InputWorkspace=__trans_summed, XMin=tof_min, XMax=tof_max, OutputWorkspace="__point")
        __trans_count = _execute("Integration", InputWorkspace=__point, OutputWorkspace="__trans_count")

        __point = _execute("CropWorkspace", InputWorkspace=sample_monitor, XMin=tof_min, XMax=tof_max, OutputWorkspace="__point")
        # __monitor_count = _execute('Integration', InputWorkspace=__point,
        #                           OutputWorkspace='__monitor_count')

        # The monitor count normalization cancels out when doing the transmission correction
        # of the scattering signal below
        __normalized_sample_trans = __trans_count  # /__monitor_count

        # Transmission workspace
        transmission = _execute(
            "Divide", LHSWorkspace=__normalized_sample_trans, RHSWorkspace=__normalized_empty, OutputWorkspace="transmission"
        )

        # Scattering signal
        __signal_summed = _execute(
            "SumSpectra", InputWorkspace=sample, StartWorkspaceIndex=0, EndWorkspaceIndex=nspecs / 2, OutputWorkspace="__signal_summed"
        )
        __point = _execute("CropWorkspace", InputWorkspace=__signal_summed, XMin=tof_min, XMax=tof_max, OutputWorkspace="__point")
        __signal_count = _execute("Integration", InputWorkspace=__point, OutputWorkspace="__signal_count")
        # The monitor count normalization cancels out when doing the transmission correction
        __signal = __signal_count  # /__monitor_count

        intensity = _execute("Divide", LHSWorkspace=__signal, RHSWorkspace=transmission, OutputWorkspace="intensity")
        return intensity


def _execute(algorithm_name, **parameters):
    alg = AlgorithmManager.create(algorithm_name)
    alg.initialize()
    alg.setChild(True)
    for key, value in parameters.items():
        if value is None:
            Logger("USANSReduction").error("Trying to set %s=None" % key)
        if alg.existsProperty(key):
            if isinstance(value, str):
                alg.setPropertyValue(key, value)
            else:
                alg.setProperty(key, value)
    try:
        alg.execute()
        if alg.existsProperty("OutputWorkspace"):
            return alg.getProperty("OutputWorkspace").value
    except:
        Logger("USANSReduction").error("Error executing [%s]" % str(alg))
        Logger("USANSReduction").error(str(sys.exc_info()[1]))
    return alg


#############################################################################################
AlgorithmFactory.subscribe(USANSReduction())
