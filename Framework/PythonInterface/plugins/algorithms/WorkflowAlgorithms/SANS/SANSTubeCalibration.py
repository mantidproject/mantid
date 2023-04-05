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


INF = sys.float_info.max  # Convenient approximation for infinity


class FuncForm(Enum):
    GAUSSIAN = 1
    EDGES = 2
    FLAT_TOP_PEAK = 3


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

    def category(self):
        return "SANS\\Calibration"

    def summary(self):
        return "Calibrates the tubes on the ISIS Sans2d Detector."

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

    def multiply_ws_list(self, ws_list, output_ws_name):
        self.log().information("Multiplying workspaces together...")
        it = iter(ws_list)
        total = str(next(it)) + "_scaled"
        for element in it:
            ws = str(element) + "_scaled"
            total = Multiply(RHSWorkspace=total, LHSWorkspace=ws, OutputWorkspace=output_ws_name)
        return total

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
        self.set_counts_to_one_between_x_range(ws, -INF, x_1)
        self.set_counts_to_one_between_x_range(ws, x_2, INF)

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

    @staticmethod
    def get_merged_edge_pairs_and_boundaries(edge_pairs):
        """Merge overlapping edge pairs, then return the merged edges and the midpoint of each edge pair."""
        # FIXME: There's probably a cleaner way to do this. ALW 2022
        boundaries = [-INF]
        edge_pairs_merged = []

        temp = edge_pairs[0]

        for start, end in sorted([sorted(edge_pair) for edge_pair in edge_pairs]):
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
        boundaries.append(INF)

        return edge_pairs_merged, boundaries

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
        self.declareProperty("Background", 10, direction=Direction.Input, doc="Baseline detector background")
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
            issues["DataFiles"] = "There must be a measurement for each strip position."
        if files > positions:
            issues["StripPositions"] = "There must be a strip position for each measurement."
        if self.getProperty("EndingPixel").value <= self.getProperty("StartingPixel").value:
            issues["EndingPixel"] = "The ending pixel must have a greater index than the starting pixel."

        return issues

    def PyExec(self):
        # Run the algorithm
        self.BACKGROUND = self.getProperty("Background").value
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
        scaled_ws_suffix = "_scaled"

        def get_proton_charge(workspace):
            proton_charge = workspace.getRun()["proton_charge_by_period"].value
            return proton_charge[0] if type(proton_charge) is np.ndarray else proton_charge

        uamphr_to_rescale = get_proton_charge(ws_list[0])
        for ws in ws_list:
            scaled_ws_name = ws.name() + scaled_ws_suffix
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
            self.set_counts_to_one_outside_x_range(mtd[ws.name() + scaled_ws_suffix], boundary_start, boundary_end)

        original_ws_name = "original"
        self.multiply_ws_list(ws_list, original_ws_name)
        result = CloneWorkspace(InputWorkspace=original_ws_name)

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

            caltable, peak_positions, meanC = self._calibrate_tube(
                ws=result,
                tube_name=tube_name,
                known_positions=known_edges,
                func_form=func_form,
                fit_params=fit_params,
                calib_table=caltable,
            )

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

        if tube_calibration_errors:
            self.log().warning("There were the following tube calibration errors:")
            for error in tube_calibration_errors:
                self.log().warning(error)

        self._notify_tube_cvalue_status(cvalues)

    def _calibrate_tube(self, ws, tube_name, known_positions, func_form, fit_params, calib_table):
        """Define the calibrated positions of the detectors inside the given tube.

        :param ws: integrated workspace with tube to be calibrated.
        :param tube_name: the name of the tube to be calibrated.
        :param known_positions: the defined position for the peaks/edges, taking the center as the origin and having the
        same units as the tube length in the 3D space.
        :param func_form: defines the format of the peaks/edge (Gaussian, Edges, Flat Top Peak).
        :param fit_params: define the parameters to be used in the fit as a TubeCalibFitParams object.
        :param calib_table: a TableWorkspace with two columns Detector ID (int) and Detector Position (V3D) to append the output values to.
        If none is provided then a new table is created.

        :rtype: the output calibration, peak and mean C table workspaces.

        """

        tube_spec = TubeSpec(ws)
        tube_spec.setTubeSpecByString(tube_name)

        ideal_tube = IdealTube()
        ideal_tube.setArray(known_positions)
        ideal_tube.setForm([func_form] * len(ideal_tube.getArray()))

        if calib_table:
            # check that the calibration table has the expected form
            if not isinstance(calib_table, ITableWorkspace):
                raise RuntimeError(
                    "Invalid type for calibTable. Expected an ITableWorkspace with 2 columns (Detector ID and Detector Position)"
                )

            if calib_table.getColumnNames() != ["Detector ID", "Detector Position"]:
                raise RuntimeError("Invalid columns for calibTable. Expected 2 columns named Detector ID and Detector Position")
        else:
            # Create the calibration table and add columns required by ApplyCalibration
            calib_table = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
            calib_table.addColumn(type="int", name="Detector ID")
            calib_table.addColumn(type="V3D", name="Detector Position")

        peak_positions, meanC = self._perform_calibration_for_tube(ws, tube_spec, calib_table, fit_params, ideal_tube)

        return calib_table, peak_positions, meanC

    def _perform_calibration_for_tube(
        self,
        ws,
        tube_spec,
        calib_table,
        fit_params,
        ideal_tube,
        excludeShortTubes=0.0,
        polinFit=2,
    ):
        """Run the calibration for the tube and put the results in the calibration table provided.

        :param ws: integrated workspace with tubes to be calibrated
        :param tube_spec: specification of the tube to be calibrated ( :class:`~tube_spec.TubeSpec` object)
        :param calib_table: the calibration table into which the
        calibrated positions of the detectors will be placed.
        :param fit_params: a :class:`~tube_calib_fit_params.TubeCalibFitParams` object for fitting the peaks
        :param ideal_tube: the :class:`~ideal_tube.IdealTube` that contains the positions in metres of the edges used for calibration
        :param excludeShortTubes: exclude tubes shorter than specified length from calibration
        :param polinFit: order of the polynomial to fit against the known positions. Acceptable: 2, 3

        """
        # The TubeSpec object will only be for one tube at a time
        tube_idx = 0
        tube_name = tube_spec.getTubeName(tube_idx)

        # Calibrate the tube, if possible
        if tube_spec.getTubeLength(tube_idx) <= excludeShortTubes:
            # skip this tube
            self.log().debug(f"Tube {tube_name} too short to calibrate.")
            return

        ws_ids, skipped = tube_spec.getTube(tube_idx)
        if len(ws_ids) < 1:
            # skip this tube
            self.log().debug(f"Cannot calibrate tube {tube_name} - unable to get any workspace indices (spectra) for it.")
            return

        # Define peak positions and calculate average of the resolution fit parameter
        peak_positions, avg_resolution = self._fit_peak_positions_for_tube(
            ws, ideal_tube.getFunctionalForms(), fit_params, ws_ids, showPlot=True
        )

        # Define the correct positions of the detectors
        calibrated_positions = self._get_calibrated_pixel_positions(ws, peak_positions, ideal_tube.getArray(), ws_ids, polinFit)
        # Check if we have corrected positions
        if len(calibrated_positions.keys()) == len(ws_ids):
            # Save the detector positions to the calibration table
            column_names = calib_table.getColumnNames()
            for det_id, new_pos in calibrated_positions.items():
                calib_table.addRow({column_names[0]: det_id, column_names[1]: new_pos})

        if skipped:
            self.log().debug("Histogram was excluded from the calibration as it did not have an assigned detector.")
        return peak_positions, avg_resolution

    def _fit_flat_top_peak(self, fit_params, point_index, ws, output_ws):
        # Find the edge position
        centre = fit_params.getPeaks()[point_index]
        outedge, inedge, endGrad = fit_params.getEdgeParameters()
        margin = fit_params.getMargin()

        # Get values around the expected center
        right_limit = len(ws.dataY(0))
        start = max(int(centre - outedge - margin), 0)
        end = min(int(centre + inedge + margin), right_limit)
        width = (end - start) / 3.0

        function = f"name=FlatTopPeak, Centre={centre}, endGrad={endGrad}, Width={width}"
        Fit(InputWorkspace=ws, Function=function, StartX=str(start), EndX=str(end), Output=output_ws)

        # peakIndex (center) is in position 1 of the parameter list -> parameter Centre of fitFlatTopPeak
        return 1

    def _fit_edges(self, fit_params, point_index, ws, output_ws):
        # Find the edge position
        centre = fit_params.getPeaks()[point_index]
        outedge, inedge, endGrad = fit_params.getEdgeParameters()
        margin = fit_params.getMargin()

        # Get values around the expected center
        all_values = ws.dataY(0)
        right_limit = len(all_values)
        values = all_values[max(int(centre - margin), 0) : min(int(centre + margin), right_limit)]

        # Identify if the edge is a sloping edge or descent edge
        descent_mode = values[0] > values[-1]
        if descent_mode:
            start = max(centre - outedge, 0)
            end = min(centre + inedge, right_limit)
            edge_mode = -1
        else:
            start = max(centre - inedge, 0)
            end = min(centre + outedge, right_limit)
            edge_mode = 1

        function = f"name=EndErfc, B={centre}, C={endGrad * edge_mode}"
        Fit(InputWorkspace=ws, Function=function, StartX=str(start), EndX=str(end), Output=output_ws)

        # peakIndex (center) is in position 1 of parameter list -> parameter B of EndERFC
        return 1

    def fitGaussian(self, fitPar, index, ws, outputWs):
        # find the peak position
        centre = fitPar.getPeaks()[index]
        margin = fitPar.getMargin()

        # get values around the expected center
        all_values = ws.dataY(0)

        RIGHTLIMIT = len(all_values)

        # RKH 18/9/19, add int()
        min_index = max(int(centre - margin), 0)
        max_index = min(int(centre + margin), RIGHTLIMIT)
        values = all_values[min_index:max_index]

        # find the peak position
        # RKH expt 7/11/19
        if fitPar.getAutomatic():
            #    Automatic=True
            #    if Automatic:
            # find the parameters for fit dynamically
            max_value = np.max(values)
            min_value = np.min(values)
            half = (max_value - min_value) * 2 / 3 + min_value
            above_half_line = len(np.where(values > half)[0])
            beyond_half_line = len(values) - above_half_line
            if above_half_line < beyond_half_line:
                # means that there are few values above the midle, so it is a peak
                centre = np.argmax(values) + min_index
                background = min_value
                height = max_value - background
                width = len(np.where(values > height / 2 + background))
            else:
                # means that there are many values above the midle, so it is a trough
                centre = np.argmin(values) + min_index
                background = max_value
                height = min_value - max_value  # negative value
                width = len(np.where(values < min_value + height / 2))

            start = max(centre - margin, 0)
            end = min(centre + margin, RIGHTLIMIT)

            fit_msg = "name=LinearBackground,A0=%f;name=Gaussian,Height=%f,PeakCentre=%f,Sigma=%f" % (background, height, centre, width)

            Fit(InputWorkspace=ws, Function=fit_msg, StartX=str(start), EndX=str(end), Output=outputWs)

            peakIndex = 3

        else:
            # get the parameters from fitParams
            background = 100
            height, width = fitPar.getHeightAndWidth()
            start = max(centre - margin, 0)
            end = min(centre + margin, RIGHTLIMIT)

            # fit the input data as a linear background + gaussian fit
            # it was seen that the best result for static general fitParamters,
            # is to divide the values in two fitting steps
            Fit(InputWorkspace=ws, Function="name=LinearBackground,A0=%f" % (background), StartX=str(start), EndX=str(end), Output="Z1")
            Fit(
                InputWorkspace="Z1_Workspace",
                Function="name=Gaussian,Height=%f,PeakCentre=%f,Sigma=%f" % (height, centre, width),
                WorkspaceIndex=2,
                StartX=str(start),
                EndX=str(end),
                Output=outputWs,
            )
            CloneWorkspace(outputWs + "_Workspace", OutputWorkspace="gauss_" + str(index))
            peakIndex = 1

        return peakIndex

    def _fit_peak_positions_for_tube(self, ws, func_forms, fit_params, ws_ids, showPlot=False):
        """
        Get the centres of N slits or edges for calibration. It looks for the peak position in pixels
        by fitting the peaks and edges. It is the method responsible for estimating the peak position in each tube.

        :param ws: workspace of integrated data
        :param func_forms: array of function form 1=slit/bar (Gaussian peak), 2=edge (pair of edges that can partly overlap), 3= FlatTopPeak
        :param fit_params: a TubeCalibFitParams object contain the fit parameters
        :param ws_ids: a list of workspace indices defining one tube
        :param showPlot: show plot for this tube

        :rtype: array of the fitted positions and average of the fit resolution parameters

        """

        # Create input workspace for fitting - get all the counts for the tube from the integrated workspace
        y_data = [ws.dataY(i)[0] for i in ws_ids]
        if len(y_data) == 0:
            return
        tube_y_data = CreateWorkspace(list(range(len(y_data))), y_data, OutputWorkspace=self._TUBE_PLOT_WS)

        calibPointWs = "CalibPoint"
        peak_positions = []
        fitt_y_values = []
        fitt_x_values = []

        avg_resolution = 0.0
        resolution_params = []

        def get_resolution_param():
            resolution = np.fabs(mtd[calibPointWs + "_Parameters"].column("Value")[2])
            if resolution > 1e-06:
                resolution_params.append(resolution)

        # Loop over the points
        for i in range(len(func_forms)):
            if func_forms[i] == FuncForm.FLAT_TOP_PEAK:
                # Find the FlatTopPeak position and save the fit resolution param to get avg resolution
                centre_param_index = self._fit_flat_top_peak(fit_params, i, tube_y_data, calibPointWs)
                get_resolution_param()
            elif func_forms[i] == FuncForm.EDGES:
                # Find the edge position and save the fit resolution param to get avg resolution
                centre_param_index = self._fit_edges(fit_params, i, tube_y_data, calibPointWs)
                get_resolution_param()
            else:
                centre_param_index = self.fitGaussian(fit_params, i, tube_y_data, calibPointWs)

            # Get the peak centre
            peak_centre = mtd[calibPointWs + "_Parameters"].column("Value")[centre_param_index]
            peak_positions.append(peak_centre)

            if showPlot:
                ws = mtd[calibPointWs + "_Workspace"]
                fitt_y_values.append(copy.copy(ws.dataY(1)))
                fitt_x_values.append(copy.copy(ws.dataX(1)))

        # Calculate the average resolution
        if resolution_params:
            avg_resolution = sum(resolution_params) / float(len(resolution_params))

        if showPlot:
            CreateWorkspace(np.hstack(fitt_x_values), np.hstack(fitt_y_values), OutputWorkspace=self._FIT_DATA_WS)

        return peak_positions, avg_resolution

    def _get_corrected_pixel_positions(self, tube_positions, known_positions, num_detectors, polinFit=2):
        """
        Corrects position errors in a tube given an array of points and their known positions.

        :param tube_positions: positions along the tube to be fitted (in pixels)
        :param known_positions: the corresponding known positions in the tube (Y-coords advised)
        :param num_detectors: number of pixel detectors in tube
        :param polinFit: order of the polynomial to fit for the ideal positions

        Return Value: array of corrected Xs (in same units as known positions)

        Note that any element of tube_positions not between 0.0 and num_detectors is ignored.
        """

        if len(tube_positions) != len(known_positions):
            self.log().debug(
                f"Number of points in tube {len(tube_positions)} must equal number of known positions in ideal tube {len(known_positions)}"
            )
            return []

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
            self.log().debug("Too few usable points in tube")
            return []

        # Fit quadratic to known positions
        PolyFittingWorkspace = CreateWorkspace(dataX=valid_tube_positions, dataY=relevant_known_positions)
        try:
            Fit(
                InputWorkspace=PolyFittingWorkspace,
                Function=f"name=Polynomial,n={polinFit}",
                StartX=str(0.0),
                EndX=str(num_detectors),
                Output="QF",
            )
        except:
            self.log().debug("Fit failed")
            return []

        # Get the fitted coefficients, excluding the last row in the parameters table because it is the error value
        coefficients = [row["Value"] for row in mtd["QF_Parameters"]][:-1]

        # Evaluate the fitted quadratic against the number of detectors
        return np.polynomial.polynomial.polyval(list(range(num_detectors)), coefficients)

    def _get_calibrated_pixel_positions(self, ws, fit_positions, known_positions, ws_ids, polinFit=2):
        """
        Get the calibrated detector positions for one tube
        The tube is specified by a list of workspace indices of its spectra
        Calibration is assumed to be done parallel to the Y-axis

        :param ws: workspace with tubes to be calibrated - may be integrated or raw
        :param fit_positions: array of calibration positions (in pixels)
        :param known_positions: where these calibration positions should be (in Y coords)
        :param ws_ids: a list of workspace indices for the tube
        :param polinFit: Order of the polinominal to fit for the ideal positions

        Return dictionary containing the pixel detector IDs and their calibrated positions
        """

        calibrated_detectors = {}

        # Get position of first and last pixel of tube
        num_detectors = len(ws_ids)
        if num_detectors < 1:
            return calibrated_detectors

        # Correct positions of detectors in tube by quadratic fit
        corrected_pixels = self._get_corrected_pixel_positions(fit_positions, known_positions, num_detectors, polinFit=polinFit)

        if len(corrected_pixels) != num_detectors:
            self.log().debug("Tube correction failed.")
            return calibrated_detectors

        # Get the detector from the base instrument, in order to get the positions before calibration
        base_instrument = ws.getInstrument().getBaseInstrument()
        first_det = base_instrument.getDetector(ws.getDetector(ws_ids[0]).getID())
        last_det = base_instrument.getDetector(ws.getDetector(ws_ids[-1]).getID())
        first_det_pos = first_det.getPos()
        last_det_pos = last_det.getPos()
        tube_length = first_det.getDistance(last_det)
        if tube_length <= 0.0:
            self.log().error("Zero length tube cannot be calibrated, calibration failed.")
            return calibrated_detectors

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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSTubeCalibration)
