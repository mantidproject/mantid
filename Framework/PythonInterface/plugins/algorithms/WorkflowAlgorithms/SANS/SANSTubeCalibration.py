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
        # Setting caltable to None  will start all over again. Set this to True to add further tubes to existing table
        caltable = None
        diagnostic_output = dict()

        # Loop through tubes to generate calibration table
        tube_report = Progress(self, start=0.4, end=0.9, nreports=120)
        tube_calibration_errors = []
        # When adding to an existing calibration table the script would have been edited to use only the required range of tubes
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

        self.notify_tube_cvalue_status(cvalues)

    def _calibrate_tube(self, ws, tube_name, known_positions, func_form, fit_params, calib_table):
        """Define the calibrated positions of the detectors inside the given tube.

        :param ws: integrated workspace with tube to be calibrated.
        :param tube_name: the name of the tube to be calibrated.
        :param known_positions: The defined position for the peaks/edges, taking the center as the origin and having the
        same units as the tube length in the 3D space.
        :param func_form: defines the format of the peaks/edge (Gaussian, Edges, Flat Top Peak).
        :param fit_params: define the parameters to be used in the fit as a TubeCalibFitParams object.
        :param calib_table: a TableWorkspace with two columns DetectorID(int) and DetectorPositions(V3D) to append the output values to.
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

    # def savePeak(peakTable, filePath):
    #     """Allows to save the peakTable to a text file.
    #
    #     :param peakTable: peak table as the workspace table provided by calibrated method, as in the example:
    #
    #     .. code-block:: python
    #
    #     calibTable, peakTable = calibrate(..., outputPeak=peakTable)
    #     savePeak(peakTable, 'myfolder/myfile.txt')
    #
    #     :param filePath: where to save the file. If the filePath is
    #     not given as an absolute path, it will be considered relative
    #     to the defaultsave.directory.
    #
    #     The file will be saved with the following format:
    #
    #     id_name (parsed space to %20) [peak1, peak2, ..., peakN]
    #
    #     You may load these peaks using readPeakFile
    #
    #     ::
    #
    #     panel1/tube001 [23.4, 212.5, 0.1]
    #     ...
    #     panel1/tubeN   [56.3, 87.5, 0.1]
    #
    #     """
    #     if not os.path.isabs(filePath):
    #         saveDirectory = config["defaultsave.directory"]
    #         pFile = open(os.path.join(saveDirectory, filePath), "w")
    #     else:
    #         pFile = open(filePath, "w")
    #     if isinstance(peakTable, str):
    #         peakTable = mtd[peakTable]
    #     nPeaks = peakTable.columnCount() - 1
    #     peaksNames = ["Peak%d" % (i + 1) for i in range(nPeaks)]
    #
    #     for line in range(peakTable.rowCount()):
    #         row = peakTable.row(line)
    #         peak_values = [row[k] for k in peaksNames]
    #         tube_name = row["TubeId"].replace(" ", "%20")
    #         print(tube_name, peak_values, file=pFile)
    #
    #     pFile.close()

    def readPeakFile(file_name):
        """Load the file calibration

        It returns a list of tuples, where the first value is the detector identification
        and the second value is its calibration values.

        Example of usage:

        .. code-block:: python

        for (det_code, cal_values) in readPeakFile('pathname/TubeDemo'):
            print det_code
            print cal_values

        :param file_name: Path for the file
        :rtype: list of tuples(det_code, peaks_values)

        """
        loaded_file = []
        # split the entries to the main values:
        # For example:
        # MERLIN/door1/tube_1_1 [34.199347724575574, 525.5864438725401, 1001.7456248836971]
        # Will be splited as:
        # ['MERLIN/door1/tube_1_1', '', '34.199347724575574', '', '525.5864438725401', '', '1001.7456248836971', '', '', '']
        pattern = re.compile(r"[\[\],\s\r]")
        saveDirectory = config["defaultsave.directory"]
        pfile = os.path.join(saveDirectory, file_name)
        for line in open(pfile, "r"):
            # check if the entry is a comment line
            if line.startswith("#"):
                continue
                # split all values
            line_vals = re.split(pattern, line)
            id_ = str(line_vals[0]).replace("%20", " ")
            if id_ == "":
                continue
            try:
                f_values = [float(v) for v in line_vals[1:] if v != ""]
            except ValueError:
                # print 'Wrong format: we expected only numbers, but receive this line ',str(line_vals[1:])
                continue

            loaded_file.append((id_, f_values))
        return loaded_file

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
        det_ids, det_positions = self.getCalibratedPixelPositions_RKH(ws, peak_positions, ideal_tube.getArray(), ws_ids, False, polinFit)
        # Check if we have corrected positions
        if len(det_ids) == len(ws_ids):
            # Save the detector positions to the calibration table
            column_names = calib_table.getColumnNames()
            for i in range(len(det_ids)):
                calib_table.addRow({column_names[0]: det_ids[i], column_names[1]: det_positions[i]})

        if skipped:
            self.log().debug("Histogram was excluded from the calibration as it did not have an assigned detector.")
        return peak_positions, avg_resolution

    def createTubeCalibtationWorkspaceByWorkspaceIndexList(
        self, integratedWorkspace, outputWorkspace, workspaceIndexList, xUnit="Pixel", showPlot=False
    ):
        """
        Creates workspace with integrated data for one tube against distance along tube
        The tube is specified by a list of workspace indices of its spectra

        @param IntegratedWorkspace: Workspace of integrated data
        @param workspaceIndexList:  list of workspace indices for the tube
        @param xUnit: unit of distance ( Pixel)
        @param showPlot: True = show plot of workspace created, False = just make the workspace.

        Return Value: Workspace created

        """

        nSpectra = len(workspaceIndexList)
        if nSpectra < 1:
            return
        pixelNumbers = []
        integratedPixelCounts = []
        pixel = 1
        # integratedWorkspace.
        for i in workspaceIndexList:
            pixelNumbers.append(pixel)
            pixel = pixel + 1
            integratedPixelCounts.append(integratedWorkspace.dataY(i)[0])

        CreateWorkspace(dataX=pixelNumbers, dataY=integratedPixelCounts, OutputWorkspace=outputWorkspace)
        # if (showPlot):
        # plotSpectrum(outputWorkspace,0)
        # For some reason plotSpectrum is not recognised, but instead we can plot this worspace afterwards.

    # Return the udet number and [x,y,z] position of the detector (or virtual detector) corresponding to spectra spectra_number
    # Thanks to Pascal Manuel for this function
    def get_detector_pos(self, work_handle, spectra_number):
        udet = work_handle.getDetector(spectra_number)
        return udet.getID(), udet.getPos()

    # Given the center of a slit in pixels return the interpolated y
    #  Converts from pixel coords to Y.
    #     If a pixel coord is not integer
    #     it is effectively rounded to half integer before conversion, rather than interpolated.
    #     It allows the pixel widths to vary (unlike correctTube).
    # Thanks to Pascal Manuel for this function
    def get_ypos(self, work_handle, pixel_float):
        center_low_pixel = int(math.floor(pixel_float))
        center_high_pixel = int(math.ceil(pixel_float))
        idlow, low = self.get_detector_pos(work_handle, center_low_pixel)  # Get the detector position of the nearest lower pixel
        idhigh, high = self.get_detector_pos(work_handle, center_high_pixel)  # Get the detector position of the nearest higher pixel
        center_y = (center_high_pixel - pixel_float) * low.getY() + (pixel_float - center_low_pixel) * high.getY()
        center_y /= center_high_pixel - center_low_pixel
        return center_y

    def fitGaussianParams(self, height, centre, sigma):  # Compose string argument for fit
        # print "name=Gaussian, Height="+str(height)+", PeakCentre="+str(centre)+", Sigma="+str(sigma)
        return "name=Gaussian, Height=" + str(height) + ", PeakCentre=" + str(centre) + ", Sigma=" + str(sigma)

    def fitEndErfcParams(self, B, C):  # Compose string argument for fit
        # print "name=EndErfc, B="+str(B)+", C="+str(C)
        return "name=EndErfc, B=" + str(B) + ", C=" + str(C)

    # RKH 11/11/19
    def fitFlatTopPeakParams(self, centre, endGrad, width):  # Compose string argument for fit
        # print "name=EndErfc, B="+str(B)+", C="+str(C)
        return "name=FlatTopPeak, Centre=" + str(centre) + ", endGrad=" + str(endGrad) + ", Width=" + str(width)

    #
    # definition of the functions to fit
    #
    # RKH 11/11/19, getting the starting parameters correct here is a MESS!
    def fitFlatTopPeak(self, fitPar, index, ws, outputWs):
        # find the edge position
        centre = fitPar.getPeaks()[index]
        outedge, inedge, endGrad = fitPar.getEdgeParameters()
        margin = fitPar.getMargin()
        # get values around the expected center
        all_values = ws.dataY(0)
        RIGHTLIMIT = len(all_values)
        # RKH 18/9/19, add int()
        # values = all_values[max(int(centre-margin),0):min(int(centre+margin),len(all_values))]

        start = max(int(centre - outedge - margin), 0)
        end = min(int(centre + inedge + margin), RIGHTLIMIT)
        width = (end - start) / 3.0
        Fit(
            InputWorkspace=ws, Function=self.fitFlatTopPeakParams(centre, endGrad, width), StartX=str(start), EndX=str(end), Output=outputWs
        )
        return 1  # peakIndex (center) is in position 1 of parameter list -> parameter B of fitFlatTopPeak

    def fitEdges(self, fitPar, index, ws, outputWs):
        # find the edge position
        centre = fitPar.getPeaks()[index]
        outedge, inedge, endGrad = fitPar.getEdgeParameters()
        margin = fitPar.getMargin()
        # get values around the expected center
        all_values = ws.dataY(0)
        RIGHTLIMIT = len(all_values)
        # RKH 18/9/19, add int()
        values = all_values[max(int(centre - margin), 0) : min(int(centre + margin), len(all_values))]

        # identify if the edge is a sloping edge or descent edge
        descentMode = values[0] > values[-1]
        if descentMode:
            start = max(centre - outedge, 0)
            end = min(centre + inedge, RIGHTLIMIT)
            edgeMode = -1
        else:
            start = max(centre - inedge, 0)
            end = min(centre + outedge, RIGHTLIMIT)
            edgeMode = 1
        Fit(
            InputWorkspace=ws, Function=self.fitEndErfcParams(centre, endGrad * edgeMode), StartX=str(start), EndX=str(end), Output=outputWs
        )
        return 1  # peakIndex (center) is in position 1 of parameter list -> parameter B of EndERFC

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
                peak_index = self.fitFlatTopPeak(fit_params, i, tube_y_data, calibPointWs)
                get_resolution_param()
            elif func_forms[i] == FuncForm.EDGES:
                # Find the edge position and save the fit resolution param to get avg resolution
                peak_index = self.fitEdges(fit_params, i, tube_y_data, calibPointWs)
                get_resolution_param()
            else:
                peak_index = self.fitGaussian(fit_params, i, tube_y_data, calibPointWs)

            # Get the peak centre
            peak_centre = mtd[calibPointWs + "_Parameters"].column("Value")[peak_index]
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

    def getIdealTubeFromNSlits(self, IntegratedWorkspace, slits):
        """
        Given N slits for calibration on an ideal tube
        convert to Y values to form a ideal tube for correctTubeToIdealTube()

        @param IntegratedWorkspace: Workspace of integrated data
        @param eP: positions of slits for ideal tube (in pixels)

        Return Value: Ideal tube in Y-coords for use by correctTubeToIdealTube()

        """
        ideal = []
        # print "slits for ideal tube", slits
        for i in range(len(slits)):
            # print slits[i]
            ideal.append(self.get_ypos(IntegratedWorkspace, slits[i]))  # Use Pascal Manuel's Y conversion.

        # print "Ideal Tube",ideal
        return ideal

    def correctTube(self, AP, BP, CP, nDets):
        """
        Corrects position errors in a tube in the same manner as is done for MERLIN
        according to an algorithm used by Rob Bewley in his MATLAB code.

        @param AP: Fit position of left (in pixels)
        @param BP: Fit position of right (in pixels)
        @param CP: Fit position of centre (in pixels)
        @param nDets: Number of pixel detectors in tube

        Return Value: Array of corrected Xs  (in pixels)
        """

        AO = AP / (nDets - AP)
        BO = (nDets - BP) / BP
        # First correct centre point for offsets
        CPN = CP - (AO * (nDets - CP)) + BO * CP
        x = []
        for i in range(nDets):
            xi = i + 1.0
            x.append(xi - ((nDets - xi) * AO) + (xi * BO))  # this is x corrected for offsets

        # Now calculate the gain error
        GainError = ((nDets + 1) / 2.0 - CPN) / (CPN * (nDets - CPN))
        xBinNew = []
        for i in range(nDets):
            xo = x[i]
            xBinNew.append(xo + (xo * (nDets - xo) * GainError))  # Final bin position values corrected for offsets and gain

        # print xBinNew
        return xBinNew

    def correctTubeToIdealTube(self, tubePoints, idealTubePoints, nDets, TestMode=False, polinFit=2):
        """
        Corrects position errors in a tube given an array of points and their ideal positions.

        :param tubePoints: Array of Slit Points along tube to be fitted (in pixels)
        :param idealTubePoints: The corresponding points in an ideal tube (Y-coords advised)
        :param nDets: Number of pixel detectors in tube
        :param Testmode: If true, detectors at the position of a slit will be moved out of the way
                            to show the reckoned slit positions when the instrument is displayed.
        :param polinFit: Order of the polinomial to fit for the ideal positions

        Return Value: Array of corrected Xs  (in same units as ideal tube points)

        Note that any element of tubePoints not between 0.0 and nDets is considered a rogue point and so is ignored.
        """

        # print "correctTubeToIdealTube"

        # Check the arguments
        if len(tubePoints) != len(idealTubePoints):
            self.log().debug(f"Number of points in tube {len(tubePoints)} must equal number of points in ideal tube {len(idealTubePoints)}")
            return xResult

        # Filter out rogue slit points
        usedTubePoints = []
        usedIdealTubePoints = []
        missedTubePoints = []  # Used for diagnostic print only
        for i in range(len(tubePoints)):
            if tubePoints[i] > 0.0 and tubePoints[i] < nDets:
                usedTubePoints.append(tubePoints[i])
                usedIdealTubePoints.append(idealTubePoints[i])
            else:
                missedTubePoints.append(i + 1)

        # State number of rogue slit points, if any
        if len(tubePoints) != len(usedTubePoints):
            self.log().debug(f"Only {len(usedTubePoints)} out of {len(tubePoints)} slit points used. Missed {missedTubePoints}")

        # Check number of usable points
        if len(usedTubePoints) < 3:
            self.log().debug(f"Too few usable points in tube {len(usedTubePoints)}")
            return []

        # Fit quadratic to ideal tube points
        CreateWorkspace(dataX=usedTubePoints, dataY=usedIdealTubePoints, OutputWorkspace="PolyFittingWorkspace")
        try:
            Fit(
                InputWorkspace="PolyFittingWorkspace",
                Function="name=Polynomial,n=%d" % (polinFit),
                StartX=str(0.0),
                EndX=str(nDets),
                Output="QF",
            )
        except:
            self.log().debug("Fit failed")
            return []

        paramQF = mtd["QF_Parameters"]

        # get the coeficients, get the Value from every row, and exclude the last one because it is the error
        # rowErr is the last one, it could be used to check accuracy of fit
        c = [r["Value"] for r in paramQF][:-1]

        # Modify the output array by the fitted quadratic
        xResult = np.polynomial.polynomial.polyval(list(range(nDets)), c)

        # In test mode, shove the pixels that are closest to the reckoned peaks
        # to the position of the first detector so that the resulting gaps can be seen.
        if TestMode:
            self.log().debug("TestMode code")
            for i in range(len(usedTubePoints)):
                # print "used point",i,"shoving pixel",int(usedTubePoints[i])
                xResult[int(usedTubePoints[i])] = xResult[0]

        # print xResult
        return xResult

    def getCalibratedPixelPositions_RKH(self, ws, tubePts, idealTubePts, whichTube, peakTestMode=False, polinFit=2):
        """
        Get the calibrated detector positions for one tube
        The tube is specified by a list of workspace indices of its spectra
        Calibration is assumed to be done parallel to the Y-axis

        :param ws: Workspace with tubes to be calibrated - may be integrated or raw
        :param tubePts: Array of calibration positions (in pixels)
        :param idealTubePts: Where these calibration positions should be (in Y coords)
        :param whichtube:  a list of workspace indices for the tube
        :param PeakTestMode: true if shoving detectors that are reckoned to be at peak away (for test purposes)
        :param polinFit: Order of the polinominal to fit for the ideal positions

        Return  Array of pixel detector IDs and array of their calibrated positions
        """

        # Arrays to be returned
        detIDs = []
        detPositions = []
        # Get position of first and last pixel of tube
        nDets = len(whichTube)
        if nDets < 1:
            return detIDs, detPositions

            # Correct positions of detectors in tube by quadratic fit
        pixels = self.correctTubeToIdealTube(tubePts, idealTubePts, nDets, TestMode=peakTestMode, polinFit=polinFit)
        # print pixels
        if len(pixels) != nDets:
            self.log().debug("Tube correction failed.")
            return detIDs, detPositions
        baseInstrument = ws.getInstrument().getBaseInstrument()
        # Get tube unit vector
        # get the detector from the baseInstrument, in order to get the positions
        # before any calibration being loaded.
        det0 = baseInstrument.getDetector(ws.getDetector(whichTube[0]).getID())
        detN = baseInstrument.getDetector(ws.getDetector(whichTube[-1]).getID())
        d0pos, dNpos = det0.getPos(), detN.getPos()
        ## identical to norm of vector: |dNpos - d0pos|
        tubeLength = det0.getDistance(detN)
        if tubeLength <= 0.0:
            self.log().error("Zero length tube cannot be calibrated, calibration failed.")
            return detIDs, detPositions
        # unfortunatelly, the operation '/' is not defined in V3D object, so
        # I have to use the multiplication.
        # unit_vectors are defined as u = (v2-v1)/|v2-v1| = (dn-d0)/length
        unit_vector = (dNpos - d0pos) * (1.0 / tubeLength)

        # Get Centre (really want to get if from IDF to allow calibration a multiple number of times)
        center = (dNpos + d0pos) * 0.5  # (1.0/2)
        # RKH 9/7/14 his does not work for our gas tubes that are not centred on X=0.0, so set the X coord to zero
        kill = V3D(0.0, 1.0, 1.0)
        center = center * kill
        # RKH added print 9/7/14
        self.log().debug(f"center={center},  unit vector= {unit_vector}")
        # Move the pixel detectors (might not work for sloping tubes)
        for i in range(nDets):
            deti = ws.getDetector(whichTube[i])
            det_pos = deti.getPos()  # noqa: F841
            pNew = pixels[i]
            # again, the opeartion float * v3d is not defined, but v3d * float is,
            # so, I wrote the new pos as center + unit_vector * (float)
            newPos = center + unit_vector * pNew

            detIDs.append(deti.getID())
            detPositions.append(newPos)
            # print i, detIDs[i], detPositions[i]

        return detIDs, detPositions

    def notify_tube_cvalue_status(self, cvalues):
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
