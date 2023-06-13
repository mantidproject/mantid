# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import copy
import itertools
import os.path
import sys
from enum import Enum

import numpy as np

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from tube_spec import TubeSpec
from ideal_tube import IdealTube
from tube_calib_fit_params import TubeCalibFitParams


class TubeSide:
    LEFT = "left"
    RIGHT = "right"

    @classmethod
    def getTubeSide(cls, tube_id):
        if tube_id % 2 == 0:
            return TubeSide.LEFT
        else:
            return TubeSide.RIGHT


class FuncForm(Enum):
    EDGES = 1
    FLAT_TOP_PEAK = 2


def pairwise(iterable):
    """Helper function from: http://docs.python.org/2/library/itertools.html:
    s -> (s0,s1), (s1,s2), (s2, s3), ...
    i.e. passing a list [s0, s1, s2, s3] to this function would return [(s0,s1), (s1,s2), (s2, s3)]"""
    a, b = itertools.tee(iterable)
    next(b, None)
    return list(zip(a, b))


class SANSTubeCalibration(PythonAlgorithm):
    _TUBE_PLOT_WS = "TubePlot"
    _FIT_DATA_WS = "FittedData"
    _SCALED_WS_SUFFIX = "_scaled"
    _CAL_TABLE_ID_COL = "Detector ID"
    _CAL_TABLE_POS_COL = "Detector Position"
    _INF = sys.float_info.max  # Acceptable approximation for infinity

    def category(self):
        return "SANS\\Calibration"

    def summary(self):
        return "Calibrates the tubes on the ISIS Sans2d Detector."

    def PyInit(self):
        # Declare properties
        self.declareProperty(
            "StripPositions", [1040, 920, 755, 590, 425, 260, 95, 5], direction=Direction.Input, doc="Which strip positions were used"
        )
        self.declareProperty(
            "DataFiles",
            ["SANS2D00064390.nxs", "SANS2D00064391.nxs", "SANS2D00064392.nxs", "SANS2D00064393.nxs", "SANS2D00064388.nxs"],
            direction=Direction.Input,
            doc="Which strip positions were used for which runs",
        )
        self.declareProperty("HalfDetectorWidth", 520.7, direction=Direction.Input)
        self.declareProperty("StripWidth", 38.0, direction=Direction.Input)
        self.declareProperty("StripToTubeCentre", 21.0, direction=Direction.Input)
        self.declareProperty("SideOffset", 0.0, direction=Direction.Input)
        self.declareProperty("EncoderAtBeamCentre", 270.0, direction=Direction.Input)
        self.declareProperty(
            "EncoderAtBeamCentreForRear260Strip",
            470.0,
            direction=Direction.Input,
            doc="Encoder at beam centre position for the 260 strip on the rear detector."
            "This is used for rear detector calibration only.",
        )
        self.declareProperty("RearDetector", True, direction=Direction.Input, doc="Whether to use the front or rear detector.")
        self.declareProperty(
            "Threshold",
            600,
            direction=Direction.Input,
            doc="Threshold is the number of counts past which we class something as an edge.  "
            "This is quite sensitive to change, since we sometimes end up picking.",
        )
        self.declareProperty(
            "SkipTubesOnEdgeFindingError",
            False,
            direction=Direction.Input,
            doc="Whether to skip calibration of tubes that we could not find the correct number of"
            "edges for. If set to False then the algorithm will terminate when it encounters a"
            "tube that it cannot find the correct number of edges for.",
        )
        self.declareProperty("Margin", 25, direction=Direction.Input, doc="FIXME: Detector margin")
        self.declareProperty("StartingPixel", 20, direction=Direction.Input, doc="Lower bound of detector's active region")
        self.declareProperty("EndingPixel", 495, direction=Direction.Input, doc="Upper bound of detector's active region")
        self.declareProperty(
            "FitEdges", False, direction=Direction.Input, doc="FIXME: Fit the full edge of a shadow, instead of just the top and bottom."
        )
        self.declareProperty("Timebins", "5000,93000,98000", direction=Direction.Input, doc="Time of flight bins to use")
        self.declareProperty("Background", 2500.0, direction=Direction.Input, doc="The baseline below the mesa for FlatTopPeak fitting")
        self.declareProperty(
            "VerticalOffset",
            -0.005,
            direction=Direction.Input,
            doc="Estimate of how many metres off-vertical the Cd strip is at bottom of the detector. "
            "Negative if strips are more to left at bottom than top of cylindrical Y plot.",
        )
        self.declareProperty(
            "CValueThreshold",
            6.0,
            direction=Direction.Input,
            doc="A notification will be logged for any tubes with a cvalue above this threshold when the calibration has completed.",
        )
        self.declareProperty(FileProperty(name="OutputFile", defaultValue="", action=FileAction.OptionalSave, extensions=["nxs"]))
        self.declareProperty(
            "SaveIntegratedWorkspaces",
            True,
            direction=Direction.Input,
            doc="Save input workspaces after loading and integrating."
            "The files will be saved to the default save location specified in your Mantid user directories.",
        )

    def validateInputs(self):
        issues = dict()
        files = len(self.getProperty("DataFiles").value)
        positions = len(self.getProperty("StripPositions").value)

        if positions > files:
            # This algorithm currently expects there to be only one strip per data file
            issues["DataFiles"] = "There must be a measurement for each strip position."
        if files > positions:
            issues["StripPositions"] = "There must be a strip position for each measurement."
        if self.getProperty("EndingPixel").value <= self.getProperty("StartingPixel").value:
            issues["EndingPixel"] = "The ending pixel must have a greater index than the starting pixel."

        return issues

    def PyExec(self):  # noqa:C901
        # Run the algorithm
        self._background = self.getProperty("Background").value
        self.timebin = self.getProperty("TimeBins").value
        margin = self.getProperty("Margin").value
        OFF_VERTICAL = self.getProperty("VerticalOffset").value
        THRESHOLD = self.getProperty("Threshold").value
        STARTPIXEL = self.getProperty("StartingPixel").value
        ENDPIXEL = self.getProperty("EndingPixel").value
        FITEDGES = self.getProperty("FitEdges").value
        self.rear = self.getProperty("RearDetector").value
        data_files = self.getProperty("DataFiles").value
        skip_tube_on_error = self.getProperty("SkipTubesOnEdgeFindingError").value
        self.outputfile = self.getProperty("OutputFile").value

        # Define the indices for the detector that we're calibrating
        if self.rear:
            index1 = 0
            index2 = 120 * 512 - 1
            detector_name = "rear"
        else:
            index1 = 120 * 512
            index2 = 2 * 120 * 512 - 1
            detector_name = "front"

        # Load calibration data
        load_report = Progress(self, start=0, end=0.4, nreports=len(data_files))
        ws_list = [self.get_integrated_workspace(data_file, load_report) for data_file in data_files]

        # Create an array of the known strip edge values for the strip positions we're using for the calibration
        known_edge_pairs = self.find_known_strip_edges(ws_list[0])
        # We want to print this information when the algorithm completes to make it easier to spot, however we capture
        # the output of the calculation here as the array may be changed later
        strip_edge_calculation_info = f"Strip edges calculated as: {known_edge_pairs}"

        # Scale workspaces
        def get_proton_charge(workspace):
            proton_charge = workspace.getRun()["proton_charge_by_period"].value
            return proton_charge[0] if type(proton_charge) is np.ndarray else proton_charge

        uamphr_to_rescale = get_proton_charge(ws_list[0])
        for ws in ws_list:
            scaled_ws_name = ws.name() + self._SCALED_WS_SUFFIX
            CropWorkspace(InputWorkspace=ws, OutputWorkspace=scaled_ws_name, StartWorkspaceIndex=index1, EndWorkspaceIndex=index2)
            Scale(
                InputWorkspace=scaled_ws_name,
                OutputWorkspace=scaled_ws_name,
                Operation="Multiply",
                Factor=uamphr_to_rescale / get_proton_charge(ws),
            )

        # Merge scaled workspaces into a single workspace containing all the strips
        known_edges_left, boundaries = self.get_merged_edge_pairs_and_boundaries(known_edge_pairs)

        # In each calibration dataset, set counts equal to 1 for x values outside the strip position merged boundaries.
        # This is so that we can multiply all the shadows together, instead of running merged workspace 5 times.
        for ws, (boundary_start, boundary_end) in zip(ws_list, pairwise(boundaries)):
            self.log().information(f"Isolating shadow in {ws} between boundaries {boundary_start} and {boundary_end}.")
            self.set_counts_to_one_outside_x_range(mtd[ws.name() + self._SCALED_WS_SUFFIX], boundary_start, boundary_end)

        merged_ws_name = "original"
        self._merge_ws_list(ws_list, merged_ws_name)
        result = CloneWorkspace(InputWorkspace=merged_ws_name)

        # Perform the calibration for each tube
        meanCvalue = []
        # Default size of a pixel in real space in mm
        default_pixel_size = (522.2 + 519.2) / 511
        caltable = None
        diagnostic_output = dict()

        # Loop through tubes to generate calibration table
        tube_report = Progress(self, start=0.4, end=0.9, nreports=120)
        tube_calibration_errors = []
        for tube_id in range(120):
            tube_name = self.get_tube_name(tube_id, detector_name)
            tube_report.report(f"Calculating tube {tube_name}")
            self.log().information("\n==================================================")
            self.log().debug(f'ID = {tube_id}, Name = "{tube_name}"')

            known_edges = []
            for edge in known_edges_left:
                known_edges.append(edge + (tube_id - 119.0) * OFF_VERTICAL / 119.0)

            guessed_pixels = list(self.get_tube_edge_pixels(detector_name, tube_id, result, THRESHOLD, STARTPIXEL, ENDPIXEL))

            if len(guessed_pixels) != len(known_edges):
                error_msg = (
                    f"Cannot calibrate tube {tube_id} - found {len(guessed_pixels)} edges when exactly {len(known_edges)} are required"
                )
                if skip_tube_on_error:
                    tube_calibration_errors.append(error_msg)
                    continue
                raise RuntimeError(error_msg)

            self.log().debug(f"Guessed pixels: {guessed_pixels}")
            self.log().debug(f"Known edges: {known_edges}")

            if FITEDGES:
                func_form = FuncForm.EDGES
                fit_params = TubeCalibFitParams(guessed_pixels, margin=margin, outEdge=10.0, inEdge=10.0)
            else:
                # Average pairs of edges for single peak fit
                guessed_avg = []
                known_avg = []
                for i in range(0, len(guessed_pixels), 2):
                    guessed_avg.append((guessed_pixels[i] + guessed_pixels[i + 1]) / 2)
                    known_avg.append((known_edges[i] + known_edges[i + 1]) / 2)
                known_edges = known_avg
                self.log().debug(f"Halved guess {guessed_avg}")
                self.log().debug(f"Halved known {known_avg}")
                func_form = FuncForm.FLAT_TOP_PEAK
                fit_params = TubeCalibFitParams(guessed_avg, height=2000, width=2 * margin, margin=margin, outEdge=10.0, inEdge=10.0)
                fit_params.setAutomatic(False)

            try:
                caltable, peak_positions, meanC = self._calibrate_tube(
                    ws=result,
                    tube_name=tube_name,
                    known_positions=known_edges,
                    func_form=func_form,
                    fit_params=fit_params,
                    calib_table=caltable,
                )
            except RuntimeError as error:
                error_msg = f"Failure attempting to calibrate tube {tube_id} - {error}"
                if skip_tube_on_error:
                    tube_calibration_errors.append(error_msg)
                    continue
                raise RuntimeError(error_msg)

            # Produce diagnostic workspaces for the tube
            diagnostic_output[tube_id] = []

            if TubeSide.getTubeSide(tube_id) == TubeSide.LEFT:
                # first pixel in mm, as per idf file for rear detector
                first_pixel_pos = -519.2
            else:
                first_pixel_pos = -522.2

            module = int(tube_id / 24) + 1
            tube_num = tube_id % 24
            ws_suffix = f"{tube_id}_{module}_{tube_num}"

            diagnostic_output[tube_id].append(RenameWorkspace(InputWorkspace=self._FIT_DATA_WS, OutputWorkspace=f"Fit{ws_suffix}"))
            diagnostic_output[tube_id].append(RenameWorkspace(InputWorkspace=self._TUBE_PLOT_WS, OutputWorkspace=f"Tube{ws_suffix}"))

            # Save the fitted positions to see how well the fit does, all in mm
            x_values = []
            x0_values = []
            peak_positions.sort()
            for i in range(len(peak_positions)):
                x0_values.append(peak_positions[i] * default_pixel_size + first_pixel_pos)
                x_values.append(known_edges[i] * 1000.0 - peak_positions[i] * default_pixel_size - first_pixel_pos)
            diagnostic_output[tube_id].append(CreateWorkspace(DataX=x0_values, DataY=x_values, OutputWorkspace=f"Data{ws_suffix}"))

            # Interrogate the calibration table to see how much we have shifted pixels for the tube
            x_values = []
            x0_values = []
            ref_pixel_pos = first_pixel_pos
            for det_pos in caltable.column("Detector Position")[-512:]:
                x_values.append(det_pos.getX() * 1000.0 - ref_pixel_pos)
                x0_values.append(ref_pixel_pos)
                ref_pixel_pos += default_pixel_size
            diagnostic_output[tube_id].append(CreateWorkspace(DataX=x0_values, DataY=x_values, OutputWorkspace=f"Shift{ws_suffix}"))

            meanCvalue.append(meanC)

        if not caltable:
            self._log_tube_calibration_issues(tube_calibration_errors)
            raise RuntimeError("Calibration failed - unable to generate calibration table")

        ApplyCalibration(result, caltable)
        cvalues = CreateWorkspace(DataX=list(diagnostic_output.keys()), DataY=meanCvalue)

        if self.outputfile:
            SaveNexusProcessed(result, self.outputfile)

        # Group the diagnostic output for each tube
        # It seems to be faster to do this here rather than as we're calibrating each tube
        for tube_id, workspaces in diagnostic_output.items():
            GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=f"Tube_{tube_id:03}")

        # Print some final status information
        self.log().notice(strip_edge_calculation_info)
        self._log_tube_calibration_issues(tube_calibration_errors)
        self._notify_tube_cvalue_status(cvalues)

    def find_known_strip_edges(self, ws):
        det_z_logname = "Rear_Det_Z" if self.rear else "Front_Det_Z"
        if not ws.run().hasProperty(det_z_logname):
            raise RuntimeError(
                f'Run log does not contain an entry for "{det_z_logname}". This is required to calculate the strip edge positions.'
            )
        sample_to_detector_dist = ws.run().getProperty(det_z_logname).firstValue()
        half_det_width = self.getProperty("HalfDetectorWidth").value
        strip_width = self.getProperty("StripWidth").value
        strip_to_tube_centre = self.getProperty("StripToTubeCentre").value
        side_offset = self.getProperty("SideOffset").value
        encoder_at_beam_centre_main = self.getProperty("EncoderAtBeamCentre").value

        def calculate_edge(encoder):
            dist_from_beam = encoder_at_beam_centre - encoder
            parallax_shift = dist_from_beam * strip_to_tube_centre / (strip_to_tube_centre - sample_to_detector_dist)
            return -(encoder + parallax_shift - half_det_width) / 1000 + side_offset

        known_edge_pairs = []
        for pos in self.getProperty("StripPositions").value:
            if self.rear and pos == 260:
                encoder_at_beam_centre = self.getProperty("EncoderAtBeamCentreForRear260Strip").value
            else:
                encoder_at_beam_centre = encoder_at_beam_centre_main
            left_edge = calculate_edge(pos + strip_width)
            right_edge = calculate_edge(pos)
            known_edge_pairs.append([left_edge, right_edge])

        return np.array(known_edge_pairs)

    def _merge_ws_list(self, ws_list, merged_ws_name):
        self.log().information("Multiplying workspaces together...")

        rhs_ws = f"{ws_list[0]}{self._SCALED_WS_SUFFIX}"

        if len(ws_list) == 1:
            # If there is only one workspace in the list then there is nothing to multiply together.
            # We re-name the workspace because we still need a workspace matching the output ws name but
            # there is no need to also keep the scaled workspace in this situation.
            RenameWorkspace(InputWorkspace=rhs_ws, OutputWorkspace=merged_ws_name)
            return

        for ws_name in ws_list[1:]:
            rhs_ws = Multiply(RHSWorkspace=rhs_ws, LHSWorkspace=f"{ws_name}{self._SCALED_WS_SUFFIX}", OutputWorkspace=merged_ws_name)

    @staticmethod
    def get_tube_name(tube_id, detector_name):
        # Construct the name of the tube based on the id (0-119) given.
        side = TubeSide.getTubeSide(tube_id)
        tube_side_num = tube_id // 2  # Need int name, not float appended
        return detector_name + "-detector/" + side + str(tube_side_num)

    def get_tube_data(self, tube_id, ws, detector_name):
        tube_name = self.get_tube_name(tube_id, detector_name)

        # Piggy-back the TubeSpec class from Karl's Calibration code
        # so that dealing with tubes is easier than interrogating the
        # IDF ourselves.
        tube_spec = TubeSpec(ws)
        tube_spec.setTubeSpecByString(tube_name)
        assert tube_spec.getNumTubes() == 1
        tube_ws_index_list = tube_spec.getTube(0)[0]
        assert len(tube_ws_index_list) == 512

        # Return an array of all counts for the tube.
        return np.array([ws.dataY(ws_index)[0] for ws_index in tube_ws_index_list])

    def get_tube_edge_pixels(self, detector_name, tube_id, ws, cutoff, first_pixel=0, last_pixel=sys.maxsize):
        count_data = self.get_tube_data(tube_id, ws, detector_name)

        if count_data[first_pixel] < cutoff:
            up_edge = True
        else:
            up_edge = False

        for i, count in enumerate(count_data[first_pixel : last_pixel + 1]):
            pixel = first_pixel + i
            if pixel > last_pixel:
                break
            if up_edge:
                if count >= cutoff:
                    up_edge = False
                    yield pixel
            else:
                if count < cutoff:
                    up_edge = True
                    yield pixel

    @staticmethod
    def set_counts_to_one_between_x_range(ws, x_1, x_2):
        """"""
        if x_1 > x_2:
            x_1, x_2 = x_2, x_1

        for wsIndex in range(ws.getNumberHistograms()):
            try:
                if x_1 < ws.getDetector(wsIndex).getPos().getX() < x_2:
                    ws.dataY(wsIndex)[0] = 1
            except RuntimeError:
                break
                # pass # Ignore "Detector with ID _____ not found" errors.

    def set_counts_to_one_outside_x_range(self, ws, x_1, x_2):
        """"""
        if x_1 > x_2:
            x_1, x_2 = x_2, x_1
        self.set_counts_to_one_between_x_range(ws, -self._INF, x_1)
        self.set_counts_to_one_between_x_range(ws, x_2, self._INF)

    def get_integrated_workspace(self, data_file, prog):
        """Load a rebin a tube calibration run.  Searched multiple levels of cache to ensure faster loading."""
        # check to see if have this file already loaded
        ws_name = os.path.splitext(data_file)[0]
        self.log().debug(f"look for: {ws_name}")
        try:
            ws = mtd[ws_name]
            self.log().information(f"Using existing {ws_name} workspace")
            prog.report(f"Loading {ws_name}")
            return ws
        except:
            pass
        try:
            ws = Load(Filename="saved_" + data_file, OutputWorkspace=ws_name)
            self.log().information(f"Loaded saved file from saved_{data_file}.")
            prog.report(f"Loading {ws_name}")
            return ws
        except:
            pass

        ws = Load(Filename=data_file, OutputWorkspace=ws_name)
        self.log().information(f"Loaded and integrating data from {data_file}.")
        # turn event mode into histogram with a single bin
        ws = Rebin(ws, self.timebin, PreserveEvents=False)
        # else for histogram data use integration or sumpsectra
        # ws = Integration(ws, OutputWorkspace=ws_name)
        if self.getProperty("SaveIntegratedWorkspaces").value:
            SaveNexusProcessed(ws, "saved_" + data_file)
        RenameWorkspace(ws, ws_name)

        prog.report(f"Loading {ws_name}")
        return ws

    def get_merged_edge_pairs_and_boundaries(self, known_edge_pairs):
        """Merge overlapping edge pairs, then return the merged edges and the midpoint of each edge pair."""
        # FIXME: There's probably a cleaner way to do this. ALW 2022
        boundaries = [-self._INF]
        edge_pairs_merged = []

        temp = known_edge_pairs[0]

        for start, end in sorted([sorted(edge_pair) for edge_pair in known_edge_pairs]):
            if start <= temp[1]:
                boundary = start + (temp[1] - start) / 2
                temp[1] = max(temp[1], end)
                if start != temp[0]:
                    boundaries.append(boundary)
            else:
                boundaries.append(temp[1] + (start - temp[1]) / 2)
                edge_pairs_merged.extend(temp)
                temp[0] = start
                temp[1] = end
        edge_pairs_merged.extend(temp)
        boundaries.append(self._INF)

        return edge_pairs_merged, boundaries

    def _calibrate_tube(self, ws, tube_name, known_positions, func_form, fit_params, calib_table):
        """Define the calibrated positions of the detectors inside the given tube.

        :param ws: integrated workspace with tube to be calibrated.
        :param tube_name: the name of the tube to be calibrated.
        :param known_positions: the defined position for the peaks/edges, taking the center as the origin and having the
        same units as the tube length in the 3D space.
        :param func_form: defines the format of the peaks/edge (Edges, Flat Top Peak).
        :param fit_params: define the parameters to be used in the fit as a TubeCalibFitParams object.
        :param calib_table: a TableWorkspace with two columns Detector ID (int) and Detector Position (V3D) to append the output values to.
        If none is provided then a new table is created.

        :rtype: the output calibration, peak and mean C table workspaces.

        """

        tube_spec = TubeSpec(ws)
        tube_spec.setTubeSpecByString(tube_name)

        ideal_tube = IdealTube()
        ideal_tube.setArray(known_positions)

        if not calib_table:
            # Create the calibration table and add columns required by ApplyCalibration
            calib_table = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
            calib_table.addColumn(type="int", name=self._CAL_TABLE_ID_COL)
            calib_table.addColumn(type="V3D", name=self._CAL_TABLE_POS_COL)

        peak_positions, meanC = self._perform_calibration_for_tube(ws, tube_spec, calib_table, func_form, fit_params, ideal_tube)

        return calib_table, peak_positions, meanC

    def _perform_calibration_for_tube(
        self,
        ws,
        tube_spec,
        calib_table,
        func_form,
        fit_params,
        ideal_tube,
    ):
        """Run the calibration for the tube and put the results in the calibration table provided.

        :param ws: integrated workspace with tubes to be calibrated
        :param tube_spec: specification of the tube to be calibrated ( :class:`~tube_spec.TubeSpec` object)
        :param calib_table: the calibration table into which the calibrated positions of the detectors will be placed.
        :param func_form: which function form to use for fitting
        :param fit_params: a :class:`~tube_calib_fit_params.TubeCalibFitParams` object for fitting the peaks
        :param ideal_tube: the :class:`~ideal_tube.IdealTube` that contains the positions in metres of the edges used for calibration

        """
        # The TubeSpec object will only be for one tube at a time
        tube_idx = 0
        tube_name = tube_spec.getTubeName(tube_idx)

        # Calibrate the tube, if possible
        if tube_spec.getTubeLength(tube_idx) <= 0.0:
            raise RuntimeError(f"Tube {tube_name} too short to calibrate.")

        ws_ids, skipped = tube_spec.getTube(tube_idx)
        if len(ws_ids) < 1:
            raise RuntimeError("Unable to get any workspace indices (spectra) for tube.")

        # Define peak positions and calculate average of the resolution fit parameter
        peak_positions, avg_resolution = self._fit_peak_positions_for_tube(ws, func_form, fit_params, ws_ids)

        # Define the correct positions of the detectors
        calibrated_positions = self._get_calibrated_pixel_positions(ws, peak_positions, ideal_tube.getArray(), ws_ids)
        # Check if we have corrected positions
        if len(calibrated_positions.keys()) == len(ws_ids):
            # Save the detector positions to the calibration table
            for det_id, new_pos in calibrated_positions.items():
                calib_table.addRow({self._CAL_TABLE_ID_COL: det_id, self._CAL_TABLE_POS_COL: new_pos})

        if skipped:
            self.log().debug("Histogram was excluded from the calibration as it did not have an assigned detector.")
        return peak_positions, avg_resolution

    def _fit_flat_top_peak(self, peak_centre, fit_params, ws, output_ws):
        # Find the position
        outedge, inedge, endGrad = fit_params.getEdgeParameters()
        margin = fit_params.getMargin()

        # Get values around the expected center
        right_limit = len(ws.dataY(0))
        start = max(int(peak_centre - outedge - margin), 0)
        end = min(int(peak_centre + inedge + margin), right_limit)
        width = (end - start) / 3.0

        function = f"name=FlatTopPeak, Centre={peak_centre}, endGrad={endGrad}, Width={width}, Background={self._background}"
        Fit(InputWorkspace=ws, Function=function, StartX=str(start), EndX=str(end), Output=output_ws)

        # peakIndex (center) is in position 1 of the parameter list -> parameter Centre of fitFlatTopPeak
        # resolutionIndex is in position 2 of the parameter list
        return 1, 2

    def _fit_edges(self, peak_centre, fit_params, ws, output_ws):
        # Find the edge position
        outedge, inedge, endGrad = fit_params.getEdgeParameters()
        margin = fit_params.getMargin()

        # Get values around the expected center
        all_values = ws.dataY(0)
        right_limit = len(all_values)
        values = all_values[max(int(peak_centre - margin), 0) : min(int(peak_centre + margin), right_limit)]

        # Identify if the edge is a sloping edge or descent edge
        descent_mode = values[0] > values[-1]
        if descent_mode:
            start = max(peak_centre - outedge, 0)
            end = min(peak_centre + inedge, right_limit)
            edge_mode = -1
        else:
            start = max(peak_centre - inedge, 0)
            end = min(peak_centre + outedge, right_limit)
            edge_mode = 1

        function = f"name=EndErfc, B={peak_centre}, C={endGrad * edge_mode}"
        Fit(InputWorkspace=ws, Function=function, StartX=str(start), EndX=str(end), Output=output_ws)

        # peakIndex (center) is in position 1 of parameter list -> parameter B of EndERFC
        # resolutionIndex is in position 2 of the parameter list
        return 1, 2

    def _fit_peak_positions_for_tube(self, ws, func_form, fit_params, ws_ids):
        """
        Get the centres of N slits or edges for calibration. It looks for the peak position in pixels
        by fitting the peaks and edges. It is the method responsible for estimating the peak position in each tube.

        :param ws: workspace of integrated data
        :param func_form: which function form to use for fitting
        :param fit_params: a TubeCalibFitParams object contain the fit parameters
        :param ws_ids: a list of workspace indices defining one tube

        :rtype: array of the fitted positions and average of the fit resolution parameters

        """

        # Create input workspace for fitting - get all the counts for the tube from the integrated workspace
        y_data = [ws.dataY(i)[0] for i in ws_ids]
        if len(y_data) == 0:
            raise RuntimeError("Cannot find any counts for the tube in the integrated workspace")

        tube_y_data = CreateWorkspace(list(range(len(y_data))), y_data, OutputWorkspace=self._TUBE_PLOT_WS)

        calibPointWs = "CalibPoint"
        peak_positions = []
        fitt_y_values = []
        fitt_x_values = []

        avg_resolution = 0.0
        resolution_params = []

        # Loop over the points
        for peak in fit_params.getPeaks():
            if func_form == FuncForm.FLAT_TOP_PEAK:
                # Find the FlatTopPeak position
                centre_param_idx, resolution_param_idx = self._fit_flat_top_peak(peak, fit_params, tube_y_data, calibPointWs)
            else:
                # Find the edge position
                centre_param_idx, resolution_param_idx = self._fit_edges(peak, fit_params, tube_y_data, calibPointWs)

            # Save the fit resolution parameter to get avg resolution
            resolution = np.fabs(mtd[calibPointWs + "_Parameters"].column("Value")[resolution_param_idx])
            if resolution > 1e-06:
                resolution_params.append(resolution)

            # Get the peak centre
            peak_centre = mtd[calibPointWs + "_Parameters"].column("Value")[centre_param_idx]
            peak_positions.append(peak_centre)

            # Calculate the values for the diagnostic workspace of fitted values
            ws = mtd[calibPointWs + "_Workspace"]
            fitt_y_values.append(copy.copy(ws.dataY(1)))
            fitt_x_values.append(copy.copy(ws.dataX(1)))

        # Calculate the average resolution
        if resolution_params:
            avg_resolution = sum(resolution_params) / float(len(resolution_params))

        # Create the diagnostic workspace of fitted values
        CreateWorkspace(np.hstack(fitt_x_values), np.hstack(fitt_y_values), OutputWorkspace=self._FIT_DATA_WS)

        return peak_positions, avg_resolution

    def _get_corrected_pixel_positions(self, tube_positions, known_positions, num_detectors):
        """
        Corrects position errors in a tube given an array of points and their known positions.

        :param tube_positions: positions along the tube to be fitted (in pixels)
        :param known_positions: the corresponding known positions in the tube (Y-coords advised)
        :param num_detectors: number of pixel detectors in tube

        Return Value: array of corrected Xs (in same units as known positions)

        Note that any element of tube_positions not between 0.0 and num_detectors is ignored.
        """

        if len(tube_positions) != len(known_positions):
            raise RuntimeError("Number of points for fitting must equal number of known positions in ideal tube.")

        # Filter out any invalid tube positions
        valid_tube_positions = []
        relevant_known_positions = []
        for i in range(len(tube_positions)):
            tube_pos = tube_positions[i]
            if 0.0 < tube_pos < num_detectors:
                valid_tube_positions.append(tube_pos)
                relevant_known_positions.append(known_positions[i])

        # Check number of usable points
        if len(valid_tube_positions) < 3:
            raise RuntimeError("Too few usable points in tube to perform fitting.")

        # Fit quadratic to known positions
        PolyFittingWorkspace = CreateWorkspace(dataX=valid_tube_positions, dataY=relevant_known_positions)
        try:
            Fit(
                InputWorkspace=PolyFittingWorkspace,
                Function="name=Polynomial,n=2",
                StartX=str(0.0),
                EndX=str(num_detectors),
                Output="QF",
            )
        except:
            raise RuntimeError("Fitting tube positions to known positions failed")

        # Get the fitted coefficients, excluding the last row in the parameters table because it is the error value
        coefficients = [row["Value"] for row in mtd["QF_Parameters"]][:-1]

        # Evaluate the fitted quadratic against the number of detectors
        return np.polynomial.polynomial.polyval(list(range(num_detectors)), coefficients)

    def _get_calibrated_pixel_positions(self, ws, fit_positions, known_positions, ws_ids):
        """
        Get the calibrated detector positions for one tube
        The tube is specified by a list of workspace indices of its spectra
        Calibration is assumed to be done parallel to the Y-axis

        :param ws: workspace with tubes to be calibrated - may be integrated or raw
        :param fit_positions: array of calibration positions (in pixels)
        :param known_positions: where these calibration positions should be (in Y coords)
        :param ws_ids: a list of workspace indices for the tube

        Return dictionary containing the pixel detector IDs and their calibrated positions
        """

        calibrated_detectors = {}

        # Get position of first and last pixel of tube
        num_detectors = len(ws_ids)
        if num_detectors < 1:
            raise RuntimeError("No detectors to calibrate for tube")

        # Correct positions of detectors in tube by quadratic fit
        corrected_pixels = self._get_corrected_pixel_positions(fit_positions, known_positions, num_detectors)

        if len(corrected_pixels) != num_detectors:
            raise RuntimeError("Number of corrected pixels for tube did not match the number of detectors.")

        # Get the detector from the base instrument, in order to get the positions before calibration
        base_instrument = ws.getInstrument().getBaseInstrument()
        first_det = base_instrument.getDetector(ws.getDetector(ws_ids[0]).getID())
        last_det = base_instrument.getDetector(ws.getDetector(ws_ids[-1]).getID())
        first_det_pos = first_det.getPos()
        last_det_pos = last_det.getPos()
        tube_length = first_det.getDistance(last_det)
        if tube_length <= 0.0:
            raise RuntimeError("Zero length tube cannot be calibrated.")

        # Get tube unit vector
        unit_vector = (last_det_pos - first_det_pos) * (1.0 / tube_length)

        # Get Centre (really want to get it from IDF to allow calibration a multiple number of times)
        center = (last_det_pos + first_det_pos) * 0.5
        # SANS2D gas tubes are not centred on X=0.0, so set the X co-ordinate to zero
        center = center * V3D(0.0, 1.0, 1.0)

        # Move the pixel detectors (might not work for sloping tubes)
        for i in range(num_detectors):
            det_id = ws.getDetector(ws_ids[i]).getID()
            new_pos = center + unit_vector * corrected_pixels[i]
            calibrated_detectors[det_id] = new_pos

        return calibrated_detectors

    def _notify_tube_cvalue_status(self, cvalues):
        all_cvalues_ok = True
        threshold = self.getProperty("CValueThreshold").value
        for i in range(len(cvalues.dataY(0))):
            cvalue = cvalues.dataY(0)[i]
            if cvalue > threshold:
                all_cvalues_ok = False
                self.log().notice(f"Tube {cvalues.dataX(0)[i]} has cvalue {cvalue}")

        if all_cvalues_ok:
            self.log().notice(f"CValues for all tubes were below threshold {threshold}")

    def _log_tube_calibration_issues(self, tube_calibration_issues):
        if tube_calibration_issues:
            self.log().warning("There were the following tube calibration errors:")
            for msg in tube_calibration_issues:
                self.log().warning(msg)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSTubeCalibration)
