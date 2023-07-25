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
from mantid.api import *  # AnalysisDataService
from mantid.simpleapi import *
from tube_spec import TubeSpec
from ideal_tube import IdealTube
from tube_calib_fit_params import TubeCalibFitParams


class TubeSide:
    LEFT = "left"
    RIGHT = "right"

    @classmethod
    def get_tube_side(cls, tube_id):
        return cls.LEFT if tube_id % 2 == 0 else cls.RIGHT

    @classmethod
    def get_first_pixel_position(cls, tube_id):
        # First pixel in mm, as per IDF for rear detector
        return -519.2 if cls.get_tube_side(tube_id) == cls.LEFT else -522.2


class DetectorInfo:
    # TODO: look these values up from the IDF
    NUM_PIXELS_IN_TUBE = 512
    NUM_TUBES = 120
    TOTAL_PIXELS = NUM_TUBES * NUM_PIXELS_IN_TUBE
    DEFAULT_PIXEL_SIZE = (522.2 + 519.2) / 511  # Default size of a pixel in real space in mm
    NUM_TUBES_PER_MODULE = 24


class FuncForm(Enum):
    EDGES = 1
    FLAT_TOP_PEAK = 2


class SANSTubeCalibration(DataProcessorAlgorithm):
    _SAVED_INPUT_DATA_PREFIX = "saved_"
    _TUBE_PLOT_WS = "__TubePlot"
    _FIT_DATA_WS = "__FittedData"
    _C_VALUES_WS = "cvalues"
    _SCALED_WS_SUFFIX = "_scaled"
    _CAL_TABLE_ID_COL = "Detector ID"
    _CAL_TABLE_POS_COL = "Detector Position"
    _CAL_TABLE_NAME = "CalibTable"
    _INF = sys.float_info.max  # Acceptable approximation for infinity
    _MERGED_WS_NAME = "original"
    _NEXUS_SUFFIX = ".nxs"

    def category(self):
        return "SANS\\Calibration"

    def summary(self):
        return "Calibrates the tubes on the ISIS Sans2d Detector."

    def PyInit(self):
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
        num_files = len(self.getProperty("DataFiles").value)
        num_positions = len(self.getProperty("StripPositions").value)

        if num_positions != len(set(self.getProperty("StripPositions").value)):
            issues["StripPositions"] = "Duplicate strip positions are not permitted."
        if num_positions > num_files:
            # This algorithm currently expects there to be only one strip per data file
            issues["DataFiles"] = "There must be a measurement for each strip position."
        if num_files > num_positions:
            issues["StripPositions"] = "There must be a strip position for each measurement."
        if self.getProperty("EndingPixel").value <= self.getProperty("StartingPixel").value:
            issues["EndingPixel"] = "The ending pixel must have a greater index than the starting pixel."
        if self.getProperty("EndingPixel").value > DetectorInfo.NUM_PIXELS_IN_TUBE:
            issues["EndingPixel"] = f"The ending pixel must be less than or equal to {DetectorInfo.NUM_PIXELS_IN_TUBE}"

        return issues

    def PyExec(self):
        self._background = self.getProperty("Background").value
        self._time_bins = self.getProperty("TimeBins").value
        margin = self.getProperty("Margin").value
        vertical_offset = self.getProperty("VerticalOffset").value
        threshold = self.getProperty("Threshold").value
        start_pixel = self.getProperty("StartingPixel").value
        end_pixel = self.getProperty("EndingPixel").value
        fit_edges = self.getProperty("FitEdges").value
        self._rear = self.getProperty("RearDetector").value
        data_files = self.getProperty("DataFiles").value
        skip_tube_on_error = self.getProperty("SkipTubesOnEdgeFindingError").value
        self._detector_name = "rear" if self._rear else "front"

        load_report = Progress(self, start=0, end=0.4, nreports=len(data_files))
        ws_list = self._load_calibration_data(data_files, load_report)

        load_report.report("Merging the datasets")

        # Calculate the known strip edge positions for each calibration dataset
        ws_to_known_edges = self._calculate_known_strip_edges(ws_list)

        # We want to print this information when the algorithm completes to make it easier to spot, but we capture
        # the output of the calculation here to ensure it's accurate
        strip_edge_calculation_info = f"Strip edges calculated as: {list(ws_to_known_edges.values())}"

        # Merge the individual datasets to get a single workspace containing all the strips
        known_edges_after_merge = self._get_strip_edges_after_merge(list(ws_to_known_edges.values()))
        self._merge_workspaces(ws_to_known_edges)

        load_report.report()

        # Perform the calibration for each tube
        alg = self.createChildAlgorithm("CloneWorkspace", InputWorkspace=self._MERGED_WS_NAME, OutputWorkspace="result")
        alg.setAlwaysStoreInADS(True)
        alg.execute()
        result = mtd["result"]

        meanCvalue = []
        caltable = None
        diagnostic_output = dict()

        # Loop through tubes to generate a calibration table
        tube_report = Progress(self, start=0.4, end=0.9, nreports=120)
        self._tube_calibration_errors = []
        for tube_id in range(120):
            tube_name = self._get_tube_name(tube_id)
            tube_report.report(f"Calculating tube {tube_name}")
            self.log().notice("\n==================================================")
            self.log().debug(f'ID = {tube_id}, Name = "{tube_name}"')

            guessed_pixels = self._find_strip_edge_pixels_for_tube(tube_id, result, threshold, start_pixel, end_pixel)

            if len(guessed_pixels) != len(known_edges_after_merge):
                error_msg = (
                    f"Cannot calibrate tube {tube_id} - "
                    f"found {len(guessed_pixels)} edges when exactly {len(known_edges_after_merge)} are required"
                )
                if skip_tube_on_error:
                    self._tube_calibration_errors.append(error_msg)
                    continue
                raise RuntimeError(error_msg)

            known_edges = self._get_known_positions_for_fitting(tube_id, known_edges_after_merge, fit_edges, vertical_offset)
            func_form = FuncForm.EDGES if fit_edges else FuncForm.FLAT_TOP_PEAK

            try:
                caltable, peak_positions, meanC = self._calibrate_tube(
                    ws=result,
                    tube_name=tube_name,
                    known_positions=known_edges,
                    func_form=func_form,
                    fit_params=self._get_fit_params(guessed_pixels, fit_edges, margin),
                    calib_table=caltable,
                )
            except RuntimeError as error:
                error_msg = f"Failure attempting to calibrate tube {tube_id} - {error}"
                if skip_tube_on_error:
                    self._tube_calibration_errors.append(error_msg)
                    continue
                raise RuntimeError(error_msg)

            diagnostic_output[tube_id] = self._create_diagnostic_workspaces(tube_id, peak_positions, known_edges, caltable)
            meanCvalue.append(meanC)

        self._apply_calibration(result, caltable)
        cvalues = self._create_workspace(data_x=list(diagnostic_output.keys()), data_y=meanCvalue, output_ws_name=self._C_VALUES_WS)

        self._save_calibrated_ws_as_nexus(result)

        # Group the diagnostic output for each tube
        # It seems to be faster to do this here rather than as we're calibrating each tube
        self._group_diagnostic_workspaces(diagnostic_output)

        # Print some final status information
        self.log().notice(strip_edge_calculation_info)
        self._log_tube_calibration_issues()
        self._notify_tube_cvalue_status(cvalues)

    def _match_workspaces_to_strip_positions(self, ws_list):
        """Match the strip positions to the workspaces"""
        strip_pos_to_ws = dict()
        for i, position in enumerate(self.getProperty("StripPositions").value):
            strip_pos_to_ws[position] = ws_list[i]

        return strip_pos_to_ws

    def _calculate_known_strip_edges(self, ws_list):
        det_z_logname = "Rear_Det_Z" if self._rear else "Front_Det_Z"
        ws = ws_list[0]

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

        strip_pos_to_ws = self._match_workspaces_to_strip_positions(ws_list)
        ws_to_known_edges = dict()
        for pos, ws in strip_pos_to_ws.items():
            if self._rear and pos == 260:
                encoder_at_beam_centre = self.getProperty("EncoderAtBeamCentreForRear260Strip").value
            else:
                encoder_at_beam_centre = encoder_at_beam_centre_main
            left_edge = calculate_edge(pos + strip_width)
            right_edge = calculate_edge(pos)
            ws_to_known_edges[ws] = [left_edge, right_edge]

        return ws_to_known_edges

    def _load_calibration_data(self, data_files, progress):
        ws_list = []

        # Define the indices for the detector that we're calibrating
        if self._rear:
            start_pixel = 0
            end_pixel = DetectorInfo.TOTAL_PIXELS - 1
        else:
            start_pixel = DetectorInfo.TOTAL_PIXELS
            end_pixel = 2 * DetectorInfo.TOTAL_PIXELS - 1

        uamphr_to_rescale = None

        for data_file in data_files:
            ws = self._get_integrated_workspace(data_file, progress)

            progress.report(f"Scaling {ws}")
            if uamphr_to_rescale is None:
                uamphr_to_rescale = self._get_proton_charge(ws)
            ws_list.append(self._crop_and_scale_workspace(ws, start_pixel, end_pixel, uamphr_to_rescale))

        return ws_list

    @staticmethod
    def _get_proton_charge(ws):
        proton_charge = ws.getRun()["proton_charge_by_period"].value
        return proton_charge[0] if type(proton_charge) is np.ndarray else proton_charge

    def _crop_and_scale_workspace(self, ws, start_pixel, end_pixel, uamphr_to_rescale):
        scaled_ws_name = ws.name() + self._SCALED_WS_SUFFIX

        crop_alg = self.createChildAlgorithm("CropWorkspace")
        crop_alg.setProperty("InputWorkspace", ws)
        crop_alg.setProperty("StartWorkspaceIndex", start_pixel)
        crop_alg.setProperty("EndWorkspaceIndex", end_pixel)
        crop_alg.execute()
        ws = crop_alg.getProperty("OutputWorkspace").value

        scale_alg = self.createChildAlgorithm("Scale")
        scale_alg.setAlwaysStoreInADS(True)
        scale_alg.setProperty("InputWorkspace", ws)
        scale_alg.setProperty("Operation", "Multiply")
        scale_alg.setProperty("Factor", uamphr_to_rescale / self._get_proton_charge(ws))
        scale_alg.setProperty("OutputWorkspace", scaled_ws_name)
        scale_alg.execute()

        return mtd[scaled_ws_name]

    def _merge_workspaces(self, ws_to_known_edges):
        """
        Merge the workspaces containing the individual strips into a single workspace containing all the strips.

        :param ws_to_known_edges: a dictionary of workspaces paired with their known strip edge locations.
        """

        # Find the boundaries that isolate the strips in each calibration dataset
        ws_to_boundaries = self._get_boundaries_for_each_strip(ws_to_known_edges)

        # This step allows us to multiply all the workspaces together in order to merge them into a single dataset.
        for ws, boundaries in ws_to_boundaries.items():
            self.log().debug(f"Isolating shadow in {ws} between boundaries {boundaries[0]} and {boundaries[1]}.")
            self._set_counts_to_one_outside_strip_boundaries(ws, boundaries)

        # Perform the merge to get a single workspace containing all the strips
        self._multiply_ws_list(list(ws_to_boundaries.keys()))

    def _multiply_ws_list(self, ws_list):
        self.log().debug("Multiplying workspaces together...")
        rhs_ws = ws_list[0]

        if len(ws_list) == 1:
            # If there is only one workspace in the list then there is nothing to multiply together.
            # We re-name the workspace because we still need a workspace matching the output ws name but
            # there is no need to also keep the scaled workspace in this situation.
            self._rename_workspace(rhs_ws, self._MERGED_WS_NAME)
            return

        for ws in ws_list[1:]:
            alg = self.createChildAlgorithm("Multiply", RHSWorkspace=rhs_ws, LHSWorkspace=ws)
            alg.execute()
            rhs_ws = alg.getProperty("OutputWorkspace").value

        AnalysisDataService.add(self._MERGED_WS_NAME, rhs_ws)

    def _get_tube_name(self, tube_id):
        """Construct the name of the tube based on the id given"""
        side = TubeSide.get_tube_side(tube_id)
        tube_side_num = tube_id // 2  # Round down to get integer name for tube
        return self._detector_name + "-detector/" + side + str(tube_side_num)

    def _get_tube_data(self, tube_id, ws):
        """Return an array of all counts for the given tube"""
        tube_name = self._get_tube_name(tube_id)

        # The TubeSpec class has been used to make this easier than interrogating the IDF
        tube_spec = TubeSpec(ws)
        tube_spec.setTubeSpecByString(tube_name)
        if not tube_spec.getNumTubes() == 1:
            raise RuntimeError(f"Found more than one tube for tube id {tube_id}")
        tube_ws_index_list = tube_spec.getTube(0)[0]
        if not len(tube_ws_index_list) == 512:
            raise RuntimeError(f"Found incorrect number of counts for tube id {tube_id}")

        return [ws.dataY(ws_index)[0] for ws_index in tube_ws_index_list]

    def _find_strip_edge_pixels_for_tube(self, tube_id, ws, threshold, first_pixel, last_pixel):
        """Finds the pixel numbers that correspond to the edges of the strips in each tube.
        The tube counts should be very low at the locations where the strips are and should be much higher outside them.
        The counts should change from high to low at the left edge of each strip, and from low to high at the right edge
        of each strip.

        :param tube_id: the ID of the tube that we want to find the strip edge pixels for.
        :param ws: the workspace containing the data.
        :param threshold: the count value that we assume distinguishes between a strip and non-strip region in the data.
        Counts above the threshold are assumed to be non-strip regions and vice versa.
        :param first_pixel: the number of the pixel at the start of the region of interest in the data.
        :param last_pixel: the number of the pixel at the end of the region of interest in the data.

        :rtype: an array of the pixel numbers that correspond to the edges of the strips in the data for the given tube.
        """

        edge_pixels = []
        count_data = self._get_tube_data(tube_id, ws)
        in_strip_region = count_data[first_pixel] < threshold

        pixel_idx = first_pixel
        for count in count_data[first_pixel:last_pixel]:
            if in_strip_region:
                if count >= threshold:
                    # Found the right edge of a strip
                    edge_pixels.append(pixel_idx)
                    in_strip_region = False
            else:
                if count < threshold:
                    # Found the left edge of a strip
                    edge_pixels.append(pixel_idx)
                    in_strip_region = True
            pixel_idx += 1

        return edge_pixels

    def _set_counts_to_one_outside_strip_boundaries(self, ws, boundaries):
        """Set counts equal to 1 for x values outside the strip position boundaries."""
        for ws_idx in range(ws.getNumberHistograms()):
            try:
                det_x = ws.getDetector(ws_idx).getPos().getX()
                if det_x < boundaries[0] or det_x > boundaries[1]:
                    ws.dataY(ws_idx)[0] = 1
            except RuntimeError:
                # Ignore detectors that can't be found in the IDF
                continue

    def _get_integrated_workspace(self, data_file, progress):
        """Load a tube calibration run. Search multiple places to ensure faster loading."""
        ws_name = os.path.splitext(data_file)[0]
        progress.report(f"Loading {ws_name}")
        self.log().debug(f"looking for: {ws_name}")

        try:
            ws = mtd[ws_name]
            self.log().notice(f"Using existing {ws_name} workspace")
            return ws
        except KeyError:
            pass

        saved_file_name = self._SAVED_INPUT_DATA_PREFIX + data_file
        try:
            alg = self.createChildAlgorithm("Load", Filename=saved_file_name, OutputWorkspace=ws_name)
            alg.setAlwaysStoreInADS(True)
            alg.execute()
            self.log().notice(f"Loaded saved file from {saved_file_name}.")
            return mtd[ws_name]
        except (ValueError, RuntimeError):
            pass

        self.log().notice(f"Loading and integrating data from {data_file}.")
        alg = self.createChildAlgorithm("Load", Filename=data_file, OutputWorkspace=ws_name)
        alg.execute()
        ws = alg.getProperty("OutputWorkspace").value

        # Turn event mode into histogram with given bins
        rebin_alg = self.createChildAlgorithm("Rebin")
        rebin_alg.setAlwaysStoreInADS(True)
        rebin_alg.setProperty("InputWorkspace", ws)
        rebin_alg.setProperty("Params", self._time_bins)
        rebin_alg.setProperty("PreserveEvents", False)
        rebin_alg.setProperty("OutputWorkspace", ws_name)
        rebin_alg.execute()
        ws = mtd[ws_name]

        if self.getProperty("SaveIntegratedWorkspaces").value:
            self._save_as_nexus(ws, saved_file_name)

        return ws

    def _get_boundaries_for_each_strip(self, ws_to_known_edges):
        """Identifies the boundaries that isolate each strip by finding the mid-point between each pair of strip edges.
        Where strips overlap, we find the mid-point of the overlapped region"""
        # Sort the strip edge positions into ascending order
        ws_to_known_edges_sorted = dict(sorted(ws_to_known_edges.items(), key=lambda entry: entry[1]))

        boundary_points = [-self._INF]
        # The right edge of the first strip is the initial reference point
        last_strip_right_edge = list(ws_to_known_edges_sorted.values())[0][1]

        def find_midpoint(first_edge, second_edge):
            return first_edge + (second_edge - first_edge) / 2

        # Loop through the rest of the strips to find the boundary points between strips
        for left_edge, right_edge in list(ws_to_known_edges_sorted.values())[1:]:
            if left_edge <= last_strip_right_edge:
                # The next strip overlaps or is right next to the last strip
                midpoint = find_midpoint(left_edge, last_strip_right_edge)
            else:
                # There is a gap between the last and next strip
                midpoint = find_midpoint(last_strip_right_edge, left_edge)
            boundary_points.append(midpoint)
            last_strip_right_edge = max(last_strip_right_edge, right_edge)

        boundary_points.append(self._INF)

        # Convert the list of boundary points into pairs of boundaries around the strips
        boundaries = self._pairwise(boundary_points)

        # Associate the boundaries with the relevant workspace
        ws_to_boundaries = dict()
        for i, ws in enumerate(ws_to_known_edges_sorted.keys()):
            ws_to_boundaries[ws] = boundaries[i]

        return ws_to_boundaries

    @staticmethod
    def _pairwise(iterable):
        """Helper function from: http://docs.python.org/2/library/itertools.html:
        Passing a list [s0, s1, s2, s3] to this function returns [(s0,s1), (s1,s2), (s2, s3)]"""
        a, b = itertools.tee(iterable)
        next(b, None)
        return list(zip(a, b))

    def _get_strip_edges_after_merge(self, known_edges):
        """Find the known edge pairs that will exist after the input workspaces have been merged."""
        merged_edge_pairs = []

        # Sort the strip edge positions into ascending order
        known_edges_sorted = sorted(known_edges)

        # The first strip is the initial reference point
        current_strip_left_edge = known_edges_sorted[0][0]
        current_strip_right_edge = known_edges_sorted[0][1]

        # Loop through the rest of the strips to find edges that will exist after merging
        for next_left_edge, next_right_edge in known_edges_sorted[1:]:
            if next_left_edge <= current_strip_right_edge:
                # The next strip overlaps or is right next to the last strip
                current_strip_right_edge = max(current_strip_right_edge, next_right_edge)
            else:
                # There is a gap between the last and next strip
                merged_edge_pairs.extend([current_strip_left_edge, current_strip_right_edge])
                current_strip_left_edge = next_left_edge
                current_strip_right_edge = next_right_edge

        merged_edge_pairs.extend([current_strip_left_edge, current_strip_right_edge])

        return merged_edge_pairs

    @staticmethod
    def _get_known_positions_for_fitting(tube_id, known_edges, fit_edges, vertical_offset):
        final_tube_id = float(DetectorInfo.NUM_TUBES - 1)

        def get_corrected_edge(edge_pos):
            return edge_pos + (tube_id - final_tube_id) * vertical_offset / final_tube_id

        if fit_edges:
            known_positions = [get_corrected_edge(edge) for edge in known_edges]
        else:
            # Average the pairs of edges for a single peak fit
            known_positions = []
            for i in range(0, len(known_edges), 2):
                known_positions.append((get_corrected_edge(known_edges[i]) + get_corrected_edge(known_edges[i + 1])) / 2)
        return known_positions

    @staticmethod
    def _get_fit_params(guessed_pixels, fit_edges, margin):
        if fit_edges:
            return TubeCalibFitParams(guessed_pixels, margin=margin, outEdge=10.0, inEdge=10.0)
        else:
            # Average the pairs of edges for a single peak fit
            guessed_avg = []
            for i in range(0, len(guessed_pixels), 2):
                guessed_avg.append((guessed_pixels[i] + guessed_pixels[i + 1]) / 2)
            fit_params = TubeCalibFitParams(guessed_avg, height=2000, width=2 * margin, margin=margin, outEdge=10.0, inEdge=10.0)
            fit_params.setAutomatic(False)
            return fit_params

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
            calib_table = self._create_calibration_table_ws()

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

    def _create_fitting_function(self, function, input_ws, start_x, end_x):
        alg = self.createChildAlgorithm("Fit")
        alg.setRethrows(True)
        alg.setProperty("Function", function)
        alg.setProperty("InputWorkspace", input_ws)
        alg.setProperty("StartX", str(start_x))
        alg.setProperty("EndX", str(end_x))
        alg.setProperty("CreateOutput", True)
        return alg

    def _run_fitting_function(self, function, input_ws, start_x, end_x):
        """
        Create and run the fitting function, returning a tuple with the two output workspaces.
        The first workspace in the tuple is the OutputParameters workspace and the second is the OutputWorkspace.
        """
        alg = self._create_fitting_function(function, input_ws, start_x, end_x)
        alg.execute()
        params_ws = alg.getProperty("OutputParameters").value
        fit_ws = alg.getProperty("OutputWorkspace").value

        return params_ws, fit_ws

    def _run_fitting_function_params_only(self, function, input_ws, start_x, end_x):
        """
        Create and run the fitting function, returning the OutputParameters workspace.
        """
        alg = self._create_fitting_function(function, input_ws, start_x, end_x)
        alg.setProperty("OutputParametersOnly", True)
        alg.execute()
        return alg.getProperty("OutputParameters").value

    def _fit_flat_top_peak(self, peak_centre, fit_params, ws):
        # Find the position
        outedge, inedge, endGrad = fit_params.getEdgeParameters()
        margin = fit_params.getMargin()

        # Get values around the expected center
        right_limit = len(ws.dataY(0))
        start = max(int(peak_centre - outedge - margin), 0)
        end = min(int(peak_centre + inedge + margin), right_limit)
        width = (end - start) / 3.0

        function = f"name=FlatTopPeak, Centre={peak_centre}, endGrad={endGrad}, Width={width}, Background={self._background}"
        params_ws, fit_ws = self._run_fitting_function(function, ws, start, end)

        # peak center is in position 1 of the parameter list -> parameter Centre of fitFlatTopPeak
        peak_centre = params_ws.column("Value")[1]

        # resolution is in position 2 of the parameter list
        resolution = np.fabs(params_ws.column("Value")[2])

        return peak_centre, resolution, fit_ws

    def _fit_edges(self, peak_centre, fit_params, ws):
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
        params_ws, fit_ws = self._run_fitting_function(function, ws, start, end)

        # peak center is in position 1 of parameter list -> parameter B of EndERFC
        peak_centre = params_ws.column("Value")[1]

        # resolution is in position 2 of the parameter list
        resolution = np.fabs(params_ws.column("Value")[2])

        return peak_centre, resolution, fit_ws

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

        tube_y_data = self._create_workspace(data_x=list(range(len(y_data))), data_y=y_data, output_ws_name=self._TUBE_PLOT_WS)

        peak_positions = []
        fitt_y_values = []
        fitt_x_values = []

        avg_resolution = 0.0
        resolution_params = []

        # Loop over the points
        for peak in fit_params.getPeaks():
            if func_form == FuncForm.FLAT_TOP_PEAK:
                # Find the FlatTopPeak position
                peak_centre, resolution, fit_ws = self._fit_flat_top_peak(peak, fit_params, tube_y_data)
            else:
                # Find the edge position
                peak_centre, resolution, fit_ws = self._fit_edges(peak, fit_params, tube_y_data)

            # Save the fit resolution parameter to get avg resolution
            if resolution > 1e-06:
                resolution_params.append(resolution)

            peak_positions.append(peak_centre)

            # Calculate the values for the diagnostic workspace of fitted values
            fitt_y_values.append(copy.copy(fit_ws.dataY(1)))
            fitt_x_values.append(copy.copy(fit_ws.dataX(1)))

        # Calculate the average resolution
        if resolution_params:
            avg_resolution = sum(resolution_params) / float(len(resolution_params))

        # Create the diagnostic workspace of fitted values
        self._create_workspace(np.hstack(fitt_x_values), np.hstack(fitt_y_values), self._FIT_DATA_WS)

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
        poly_fitting_workspace = self._create_workspace(
            valid_tube_positions, relevant_known_positions, "PolyFittingWorkspace", store_in_ADS=False
        )
        try:
            fitted_params = self._run_fitting_function_params_only(
                function="name=Polynomial,n=2",
                input_ws=poly_fitting_workspace,
                start_x=str(0.0),
                end_x=str(num_detectors),
            )
        except:
            raise RuntimeError("Fitting tube positions to known positions failed")

        # Get the fitted coefficients, excluding the last row in the parameters table because it is the error value
        coefficients = [row["Value"] for row in fitted_params][:-1]

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

    def _create_diagnostic_workspaces(self, tube_id, peak_positions, known_edges, caltable):
        """Produce diagnostic workspaces for the tube"""
        diagnostic_workspaces = []

        first_pixel_pos = TubeSide.get_first_pixel_position(tube_id)
        module = int(tube_id / DetectorInfo.NUM_TUBES_PER_MODULE) + 1
        tube_num = tube_id % DetectorInfo.NUM_TUBES_PER_MODULE
        ws_suffix = f"{tube_id}_{module}_{tube_num}"

        diagnostic_workspaces.append(self._rename_workspace(self._FIT_DATA_WS, f"Fit{ws_suffix}"))
        diagnostic_workspaces.append(self._rename_workspace(self._TUBE_PLOT_WS, f"Tube{ws_suffix}"))

        # Save the fitted positions to see how well the fit does, all in mm
        known_positions = []
        fitted_positions = []
        peak_positions.sort()
        for i in range(len(peak_positions)):
            fitted_positions.append(peak_positions[i] * DetectorInfo.DEFAULT_PIXEL_SIZE + first_pixel_pos)
            known_positions.append(known_edges[i] * 1000.0 - peak_positions[i] * DetectorInfo.DEFAULT_PIXEL_SIZE - first_pixel_pos)
        diagnostic_workspaces.append(
            self._create_workspace(data_x=fitted_positions, data_y=known_positions, output_ws_name=f"Data{ws_suffix}")
        )

        # Interrogate the calibration table to see how much we have shifted pixels for the tube
        calibrated_shift = []
        ref_positions = []
        ref_pixel_pos = first_pixel_pos
        for det_pos in caltable.column(self._CAL_TABLE_POS_COL)[-DetectorInfo.NUM_PIXELS_IN_TUBE :]:
            calibrated_shift.append(det_pos.getX() * 1000.0 - ref_pixel_pos)
            ref_positions.append(ref_pixel_pos)
            ref_pixel_pos += DetectorInfo.DEFAULT_PIXEL_SIZE
        diagnostic_workspaces.append(
            self._create_workspace(data_x=ref_positions, data_y=calibrated_shift, output_ws_name=f"Shift{ws_suffix}")
        )

        return diagnostic_workspaces

    def _save_calibrated_ws_as_nexus(self, calibrated_ws):
        output_file = self.getProperty("OutputFile").value
        if output_file:
            save_filepath = output_file if output_file.endswith(self._NEXUS_SUFFIX) else f"{output_file}{self._NEXUS_SUFFIX}"
            self._save_as_nexus(calibrated_ws, save_filepath)

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

    def _log_tube_calibration_issues(self):
        if self._tube_calibration_errors:
            self.log().warning("There were the following tube calibration errors:")
            for msg in self._tube_calibration_errors:
                self.log().warning(msg)

    def _create_workspace(self, data_x, data_y, output_ws_name, store_in_ADS=True):
        alg = self.createChildAlgorithm("CreateWorkspace")
        alg.setAlwaysStoreInADS(store_in_ADS)
        alg.setProperty("DataX", data_x)
        alg.setProperty("DataY", data_y)
        alg.setProperty("OutputWorkspace", output_ws_name)
        alg.execute()

        if store_in_ADS:
            return mtd[output_ws_name]
        else:
            return alg.getProperty("OutputWorkspace").value

    def _rename_workspace(self, input_ws, new_name):
        alg = self.createChildAlgorithm("RenameWorkspace", InputWorkspace=input_ws, OutputWorkspace=new_name)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _create_calibration_table_ws(self):
        """Create the calibration table and add columns required by ApplyCalibration"""
        alg = self.createChildAlgorithm("CreateEmptyTableWorkspace", OutputWorkspace=self._CAL_TABLE_NAME)
        alg.setAlwaysStoreInADS(True)
        alg.execute()

        calib_table = mtd[self._CAL_TABLE_NAME]
        calib_table.addColumn(type="int", name=self._CAL_TABLE_ID_COL)
        calib_table.addColumn(type="V3D", name=self._CAL_TABLE_POS_COL)
        return calib_table

    def _apply_calibration(self, ws_to_calibrate, caltable):
        """Apply the generated calibration table"""
        if not caltable:
            self._log_tube_calibration_issues()
            raise RuntimeError("Calibration failed - unable to generate calibration table")

        cal_alg = self.createChildAlgorithm("ApplyCalibration", Workspace=ws_to_calibrate, CalibrationTable=caltable)
        cal_alg.execute()

    def _group_diagnostic_workspaces(self, diagnostic_output):
        """Group the diagnostic output for each tube"""
        alg = self.createChildAlgorithm("GroupWorkspaces")
        alg.setAlwaysStoreInADS(True)
        for tube_id, workspaces in diagnostic_output.items():
            alg.setProperty("InputWorkspaces", workspaces)
            alg.setProperty("OutputWorkspace", f"Tube_{tube_id:03}")
            alg.execute()

    def _save_as_nexus(self, ws, filename):
        save_alg = self.createChildAlgorithm("SaveNexusProcessed", InputWorkspace=ws, Filename=filename)
        save_alg.execute()


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSTubeCalibration)
