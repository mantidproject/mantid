import copy
import itertools
import os.path
import sys

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


def pairwise(iterable):
    """Helper function from: http://docs.python.org/2/library/itertools.html:
    s -> (s0,s1), (s1,s2), (s2, s3), ..."""
    a, b = itertools.tee(iterable)
    next(b, None)
    return list(zip(a, b))


class Calibrate(PythonAlgorithm):
    _strip_edges = {
        1040: [-0.562365234, -0.524046455],
        920: [-0.44052572, -0.402347555],
        755: [-0.27475211, -0.236573945],
        590: [-0.1089785, -0.070800335],
        425: [0.056795111, 0.094973275],
        260: [0.22350643, 0.261684595],
        95: [0.388342331, 0.426520496],
        5: [0.4787643, 0.516942465]
    }

    def multiply_ws_list(self, ws_list, output_ws_name):
        self.log().information("Multiplying workspaces together...")
        it = iter(ws_list)
        total = str(next(it)) + '_scaled'
        for element in it:
            ws = str(element) + '_scaled'
            total = Multiply(RHSWorkspace=total,
                             LHSWorkspace=ws,
                             OutputWorkspace=output_ws_name)
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
        return np.array(
            [ws.dataY(ws_index)[0] for ws_index in tube_ws_index_list])

    def get_tube_edge_pixels(self,
                             detector_name,
                             tube_id,
                             ws,
                             cutoff,
                             first_pixel=0,
                             last_pixel=sys.maxsize):
        count_data = self.get_tube_data(tube_id, ws, detector_name)

        if count_data[first_pixel] < cutoff:
            up_edge = True
        else:
            up_edge = False

        for i, count in enumerate(count_data[first_pixel:last_pixel + 1]):
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
            self.log().information(f"Loaded saved file froms aved_{data_file}.")
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
        SaveNexusProcessed(ws, "saved_" + data_file)
        RenameWorkspace(ws, ws_name)

        prog.report(f"Loading {ws_name}")
        return ws

    @staticmethod
    def get_merged_edge_pairs_and_boundaries(edge_pairs):
        """Merge overlapping edge pairs, then return the merged edges and the midpoint of each edge pair."""
        #FIXME: There's probably a cleaner way to do this. ALW 2022
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
                edge_pairs_merged.append(tuple(temp))
                temp[0] = start
                temp[1] = end
        edge_pairs_merged.append(tuple(temp))
        boundaries.append(INF)

        return edge_pairs_merged, boundaries

    def category(self):
        return 'SANS\\TubeCalibration'

    def PyInit(self):
        # Declare properties
        self.declareProperty('StripPositions',
                             [1040, 920, 755, 590, 425, 260, 95, 5],
                             direction=Direction.Input,
                             doc="Which strip positions were used")
        self.declareProperty(
            'DataFiles', [
                "SANS2D00064390.nxs", "SANS2D00064391.nxs",
                "SANS2D00064392.nxs", "SANS2D00064393.nxs",
                "SANS2D00064388.nxs"
            ],
            direction=Direction.Input,
            doc="Which strip positions were used for which runs")
        self.declareProperty('RearDetector',
                             True,
                             direction=Direction.Input,
                             doc="Whether to use the front or rear detector.")
        self.declareProperty(
            'Threshold',
            600,
            direction=Direction.Input,
            doc= "Threshold is the number of counts past which we class something as an edge.  "
            "This is quite sensitive to change, since we sometimes end up picking."
        )
        self.declareProperty('Margin',
                             25,
                             direction=Direction.Input,
                             doc="FIXME: Detector margin")
        self.declareProperty('StartingPixel',
                             20,
                             direction=Direction.Input,
                             doc="Lower bound of detector's active region")
        self.declareProperty('EndingPixel',
                             495,
                             direction=Direction.Input,
                             doc="Upper bound of detector's active region")
        self.declareProperty(
            'FitEdges',
            False,
            direction=Direction.Input,
            doc=
            "FIXME: Fit the full edge of a shadow, instead of just the top and bottom."
        )
        self.declareProperty('Timebins',
                             '5000,93000,98000',
                             direction=Direction.Input,
                             doc="Time of flight bins to use")
        self.declareProperty('Background',
                             10,
                             direction=Direction.Input,
                             doc="Baseline detector background")
        self.declareProperty(
            'VerticalOffset',
            -0.005,
            direction=Direction.Input,
            doc=
            "Estimate of how many metres off-vertical the Cd strip is at bottom of the detector. "
            "Negative if strips are more to left at bottom than top of cylindrical Y plot."
        )

    def validateInputs(self):
        issues = dict()
        files = len(self.getProperty("DataFiles").value)
        positions = len(self.getProperty("StripPositions").value)

        if positions > files:
            issues[
                "DataFiles"] = "There must be a measurement for each strip position."
        if files > positions:
            issues[
                "StripPositions"] = "There must be a strip position for each measurement."
        if self.getProperty("EndingPixel").value <= self.getProperty(
                "StartingPixel").value:
            issues[
                "EndingPixel"] = "The ending pixel must have a greater index than the starting pixel."

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
        known_edge_pairs = np.array([
            self._strip_edges[pos]
            for pos in self.getProperty("StripPositions").value
        ])

        if self.rear:
            index1 = 0
            index2 = 120 * 512 - 1
            detector_name = "rear"
        else:
            index1 = 120 * 512
            index2 = 2 * 120 * 512 - 1
            detector_name = "front"

        load_report = Progress(self,
                               start=0,
                               end=0.4,
                               nreports=len(data_files))
        ws_list = [
            self.get_integrated_workspace(data_file, load_report)
            for data_file in data_files
        ]

        # Scale workspaces
        i = 0

        def charge(ws):
            return mtd[ws].getRun()["proton_charge_by_period"].value

        uamphr_to_rescale = charge(data_files[0].split('.')[0])
        for ws in data_files:
            ws2 = ws.split('.')[0]
            CropWorkspace(InputWorkspace=ws2,
                          OutputWorkspace=ws2 + '_scaled',
                          StartWorkspaceIndex=index1,
                          EndWorkspaceIndex=index2)
            Scale(uamphr_to_rescale / charge(ws2),
                  "Multiply",
                  InputWorkspace=ws2 + '_scaled',
                  OutputWorkspace=ws2 + '_scaled')
            i += 1

        known_left_edge_pairs = copy.copy(known_edge_pairs)

        _, boundaries = self.get_merged_edge_pairs_and_boundaries(
            known_edge_pairs)
        known_left_edges, _ = self.get_merged_edge_pairs_and_boundaries(
            known_left_edge_pairs)

        for ws, (boundary_start, boundary_end) in zip(ws_list,
                                                      pairwise(boundaries)):
            self.log().information(f"Isolating shadow in {ws} between boundaries {boundary_start} and {boundary_end}.")
            # set to 1 so that we can multiply all the shadows together, instead of running merged workspace 5 times.
            ws2 = str(ws) + '_scaled'
            self.set_counts_to_one_outside_x_range(mtd[ws2], boundary_start,
                                                   boundary_end)

        result_ws_name = "result"

        self.multiply_ws_list(ws_list, result_ws_name)

        result = mtd[result_ws_name]

        original = CloneWorkspace(InputWorkspace=result_ws_name,
                                  OutputWorkspace="original")

        known_edges_left = list(
            itertools.chain.from_iterable(known_left_edges))
        failed_pixel_guesses = []
        pixel_guesses = []
        meanCvalue = []
        tubeList = []

        dx = (522.2 + 519.2) / 511
        # default size of a pixel in real space in mm
        # setting caltable to None  will start all over again, comment this line out to add further tubes to existing table
        caltable = None
        # caltable = True
        diag_output = dict()

        tube_report = Progress(self, start=0.4, end=0.9, nreports=120)
        for tube_id in range(120):
            # for tube_id in range(116,120):
            diag_output[tube_id] = []

            tube_name = self.get_tube_name(tube_id, detector_name)
            tube_report.report(f"Calculating tube {tube_name}")
            self.log().information("\n==================================================")
            self.log().debug(f'ID = {tube_id}, Name = "{tube_name}"')
            known_edges1 = copy.copy(known_edges_left)
            if TubeSide.getTubeSide(tube_id) == TubeSide.LEFT:
                # first pixel in mm, as per idf file for rear detector
                x0 = -519.2
            else:
                x0 = -522.2

            np.array(known_edges1)

            known_edges = []
            for index in range(len(known_edges1)):
                known_edges.append(known_edges1[index]
                                   + (tube_id - 119.0) * OFF_VERTICAL / 119.0)

            guessed_pixels = list(
                self.get_tube_edge_pixels(detector_name, tube_id, result,
                                          THRESHOLD, STARTPIXEL, ENDPIXEL))

            # Store the guesses for printing out later, along with the tube id and name.
            # pixel_guesses.append([tube_name, guessed_pixels])

            self.log().debug(f"{len(guessed_pixels)} {guessed_pixels}")
            self.log().debug(f"{len(known_edges)} {known_edges}")

            # note funcForm==2 fits an edge using error function, (see
            # SANS2DEndErfc above, and code in tube_calib_RKH.py,)
            # while any other value fits a Gaussian
            if FITEDGES:
                funcForm = [2] * len(guessed_pixels)
                fitPar = TubeCalibFitParams(guessed_pixels,
                                            outEdge=10.0,
                                            inEdge=10.0)
            else:
                # average pairs of edges for single peak fit, could in principle do only some tubes or parts of tubes this way!
                guess = []
                known = []
                for i in range(0, len(guessed_pixels), 2):
                    guess.append(
                        (guessed_pixels[i] + guessed_pixels[i + 1]) / 2)
                    known.append((known_edges[i] + known_edges[i + 1]) / 2)
                funcForm = [3] * len(guess)
                guessed_pixels = guess
                known_edges = known
                fitPar = TubeCalibFitParams(guessed_pixels,
                                            height=2000,
                                            width=2 * margin,
                                            margin=margin,
                                            outEdge=10.0,
                                            inEdge=10.0)
                fitPar.setAutomatic(False)
                self.log().debug(f"halved guess {len(guessed_pixels)} {guessed_pixels}")
                self.log().debug(f"halved known {len(known_edges)} {known_edges}")

            module = int(tube_id / 24) + 1
            tube_num = tube_id - (module - 1) * 24
            self.log().debug(f"module {module}   tube {tube_num}")

            if caltable:
                # does this after the first time, it appends to a table
                caltable, peakTable, meanCTable = self.calibrate(
                    result,
                    tube_name,
                    np.array(known_edges),
                    funcForm,
                    # outputPeak=peakTable falls over at addrow if number of peaks changes, so can only save one row at a time
                    outputPeak=True,
                    outputC=True,
                    rangeList=[0],
                    plotTube=[0],
                    margin=margin,
                    fitPar=fitPar,
                    calibTable=caltable)
            else:
                # do this the FIRST time, starts a new table
                self.log().debug("first time, generate calib table")
                caltable, peakTable, meanCTable = self.calibrate(
                    result,
                    tube_name,
                    np.array(known_edges),
                    funcForm,
                    # outputPeak=True,
                    rangeList=[0],
                    plotTube=[0],
                    outputPeak=True,
                    outputC=True,
                    margin=margin,
                    fitPar=fitPar)
            diag_output[tube_id].append(
                CloneWorkspace(InputWorkspace="FittedTube0",
                               OutputWorkspace=f"Fit{tube_id}_{module}_{tube_num}"))
            diag_output[tube_id].append(
                CloneWorkspace(InputWorkspace="TubePlot0",
                               OutputWorkspace=f"Tube{tube_id}_{module}_{tube_num}"))
            # 8/7/14 save the fitted positions to see how well the fit does, all in mm
            x_values = []
            x0_values = []
            bb = list(mtd["PeakTable"].row(0).values())
            del bb[0]  # Remove string that can't be sorted
            bb.sort()
            # bb still contains a name string at the end
            # it's just for disgnostics
            for i in range(len(guessed_pixels)):
                x0_values.append(bb[i] * dx + x0)
                x_values.append(known_edges[i] * 1000. - bb[i] * dx - x0)
            cc = CreateWorkspace(DataX=x0_values, DataY=x_values)
            diag_output[tube_id].append(
                RenameWorkspace(InputWorkspace=cc,
                                OutputWorkspace=f"Data{tube_id}_{module}_{tube_num}"))

            bb = list(mtd["meanCTable"].row(0).values())
            meanCvalue.append(bb[1])
            tubeList.append(tube_id)

        ApplyCalibration(result, caltable)
        self.log().debug(str(tubeList))
        self.log().debug(str(meanCvalue))
        cvalues = CreateWorkspace(DataX=tubeList, DataY=meanCvalue)

        # FIXME: This needs more information in the file name
        outputfilename = "TubeCalirbationTable_512pixel.nxs"
        self.log().debug(outputfilename)
        SaveNexusProcessed(result, outputfilename)
        # you will next need to run merge_calib_files.py to merge the tables for front and rear detectors

        # expts
        aa = mtd["PeakTable"].row(0)
        self.log().debug(str(aa))
        self.log().debug(str(aa.get('Peak10')))
        bb = list(aa.values())

        bb = list(mtd["PeakTable"].row(0).values())
        del bb[0]  # Remove string that can't be sorted
        self.log().debug(str(bb))
        bb.sort()
        self.log().debug(str(bb))

        # now interrogate CalibTable to see how much we have shifted pixels by for each tube
        # 18/3/15 this new version will work when we have skipped tubes as it reads the Detector ID's  from the table itself
        # All this creates ws to check results, doesn't affect the main code
        nentries = int(len(mtd["CalibTable"]) / 512)
        self.log().information(f"nentries in CalibTable = {nentries}")
        i1 = 0
        i2 = 512
        dx = (522.2 + 519.2) / 511
        calib_report = Progress(self, start=0.9, end=1.0, nreports=nentries)
        for i in range(0, nentries):
            tube_num = mtd["CalibTable"].column("Detector ID")[i1]
            tube_num /= 1000
            det = int(tube_num / 1000)
            tube_num -= det * 1000
            module = int(tube_num / 100)
            tube_num = tube_num - module * 100
            tube_id = (module - 1) * 24 + tube_num
            self.log().debug(f"{tube_id} {module} {tube_num}")
            x_values = []
            x0_values = []
            # use left tube value here for now, right tube starts at -522.2    WHY THIS HERE ????
            if TubeSide.getTubeSide(tube_id) == TubeSide.LEFT:
                x0 = -519.2
            else:
                x0 = -522.2
            for pos in mtd["CalibTable"].column("Detector Position")[i1:i2]:
                x_values.append(pos.getX() * 1000.0 - x0)
                x0_values.append(x0)
                x0 += dx
            plotN = CreateWorkspace(DataX=x0_values, DataY=x_values)
            diag_output[i].append(
                RenameWorkspace(InputWorkspace="plotN",
                                OutputWorkspace=f"Shift{tube_id}_{module}_{tube_num}"))
            i1 = i1 + 512
            i2 = i2 + 512
            calib_report.report("Calibrating")

        for tube_id, workspaces in diag_output.items():
            GroupWorkspaces(InputWorkspaces=workspaces,
                            OutputWorkspace=f"Tube_{tube_id:03}")

        for x in (i for j in (list(range(0, 2)), list(range(10, 12)),
                              list(range(23, 29))) for i in j):
            print(x)
            # Notice to self, the result will look wiggly in 3D but looks good in cylindrical Y

    def calibrate(self, ws, tubeSet, knownPositions, funcForm, **kwargs):
        """Define the calibrated positions of the detectors inside the tubes defined
        in tubeSet.

        Tubes may be considered a list of detectors alined that may be considered
        as pixels for the analogy when they values are displayed.

        The position of these pixels are provided by the manufactor, but its real
        position depends on the electronics inside the tube and varies slightly
        from tube to tube. The calibrate method, aims to find the real positions
        of the detectors (pixels) inside the tube.

        For this, it will receive an Integrated workspace, where a special
        measurement was performed so to have a
        pattern of peaks or through. Where gaussian peaks or edges can be found.


        The calibration follows the following steps

        1. Finding the peaks on each tube
        2. Fitting the peaks agains the Known Positions
        3. Defining the new position for the pixels(detectors)

        Let's consider the simplest way of calling calibrate:

        .. code-block:: python

            from tube import calibrate
            ws = Load('WISH17701')
            ws = Integration(ws)
            known_pos = [-0.41,-0.31,-0.21,-0.11,-0.02, 0.09, 0.18, 0.28, 0.39 ]
            peaks_form = 9*[1] # all the peaks are gaussian peaks
            calibTable = calibrate(ws,'WISH/panel03',known_pos, peaks_form)

        In this example, the calibrate framework will consider all the
        tubes (152) from WISH/panel03.
        You may decide to look for a subset of the tubes, by passing the
        **rangeList** option.

        .. code-block:: python

            # This code will calibrate only the tube indexed as number 3
            # (usually tube0004)
            calibTable = calibrate(ws,'WISH/panel03',known_pos,
                                    peaks_form, rangeList=[3])

        **Finding the peaks on each tube**

        * Dynamically fitting peaks

        The framework expects that for each tube, it will find a peak pattern
        around the pixels corresponding to the known_pos positions.

        The way it will work out the estimated peak position (in pixel) is

        1. Get the length of the tube: distance(first_detector,last_detector) in the tube.
        2. Get the number of detectors in the tube (nDets)
        3. It will be assumed that the center of the tube correspond to the origin (0)

        .. code-block:: python

            centre_pixel = known_pos * nDets/tube_length + nDets/2

        It will them look for the real peak around the estimated value as:

        .. code-block:: python

            # consider tube_values the array of counts, and peak the estimated
            # position for the peak
            real_peak_pos = argmax(tube_values[peak-margin:peak+margin])

        After finding the real_peak_pos, it will try to fit the region around
        the peak to find the best expected position of the peak in a continuous
        space. It will do this by fitting the region around the peak to a
        Gaussian Function, and them extract the PeakCentre returned by the
        Fitting.

        .. code-block:: python

            centre = real_peak_pos
            fit_start, fit_stop = centre-margin, centre+margin
            values = tube_values[fit_start,fit_stop]
            background = min(values)
            peak = max(values) - background
            width = len(where(values > peak/2+background))
            # It will fit to something like:
            # Fit(function=LinerBackground,A0=background;Gaussian,
            # Height=peak, PeakCentre=centre, Sigma=width,fit_start,fit_end)

        * Force Fitting Parameters


        These dinamically values can be avoided by defining the **fitPar** for
        the calibrate function

        .. code-block:: python

            eP = [57.5, 107.0, 156.5, 206.0, 255.5, 305.0, 354.5, 404.0, 453.5]
            # Expected Height of Gaussian Peaks (initial value of fit parameter)
            ExpectedHeight = 1000.0
            # Expected width of Gaussian peaks in pixels
            # (initial value of fit parameter)
            ExpectedWidth = 10.0
            fitPar = TubeCalibFitParams( eP, ExpectedHeight, ExpectedWidth )
            calibTable = calibrate(ws, 'WISH/panel03', known_pos, peaks_form, fitPar=fitPar)

        Different Function Factors


        Although the examples consider only Gaussian peaks, it is possible to
        change the function factors to edges by passing the index of the
        known_position through the **funcForm**. Hence, considering three special
        points, where there are one gaussian peak and thow edges, the calibrate
        could be configured as:

        .. code-block:: python

            known_pos = [-0.1 2 2.3]
            # gaussian peak followed by two edges (through)
            form_factor = [1 2 2]
            calibTable = calibrate(ws,'WISH/panel03',known_pos,
                                    form_factor)

        * Override Peaks


        It is possible to scape the finding peaks position steps by providing the
        peaks through the **overridePeaks** parameters. The example below tests
        the calibration of a single tube (30) but scapes the finding peaks step.

        .. code-block:: python

            known_pos = [-0.41,-0.31,-0.21,-0.11,-0.02, 0.09, 0.18, 0.28, 0.39 ]
            define_peaks = [57.5, 107.0, 156.5, 206.0, 255.5, 305.0, 354.5,
                            404.0, 453.5]
            calibTable = calibrate(ws, 'WISH/panel03', known_pos, peaks_form,
                            overridePeaks={30:define_peaks}, rangeList=[30])

        * Output Peaks Positions

        Enabling the option **outputPeak** a WorkspaceTable will be produced with
        the first column as tube name and the following columns with the position
        where corresponding peaks were found. Like the table below.

        +-------+-------+-----+-------+
        |TubeId | Peak1 | ... | PeakM |
        +=======+=======+=====+=======+
        |tube0  | 15.5  | ... | 370.3 |
        +-------+-------+-----+-------+
        |  ...  |  ...  | ... |  ...  |
        +-------+-------+-----+-------+
        |tubeN  | 14.9  | ... | 371.2 |
        +-------+-------+-----+-------+

        The signature changes to:

        .. code-block:: python

            calibTable, peakTable = calibrate(...)

        It is possible to give a peakTable directly to the **outputPeak** option,
        which will make the calibration to append the peaks to the given table.

        .. hint::

            It is possible to save the peakTable to a file using the
            :meth:`savePeak` method.

        **Find the correct position along the tube**


        The second step of the calibration is to define the correct position of
        pixels along the tube. This is done by fitting the peaks positions found
        at the previous step against the known_positions provided.

        ::

            known       |              *
            positions   |           *
                        |      *
                        |  *
                        |________________
                        pixels positions

        The default operation is to fit the pixels positions against the known
        positions with a quadratic function in order to define an operation to
        move all the pixels to their real positions. If necessary, the user may
        select to fit using a polinomial of 3rd order, through the parameter
        **fitPolyn**.

        .. note::

            The known positions are given in the same unit as the spacial position
            (3D) and having the center of the tube as the origin.

        Hence, this section will define a function that:

        .. math:: F(pix) = RealRelativePosition

        **Define the new position for the detectors**

        Finally, the position of the detectors are defined as a vector operation
        like

        .. math::

            \\vec{p} = \\vec{c} + v \\vec{u}

        Where :math:`\\vec{p}` is the position in the 3D space, **v** is the
        RealRelativePosition deduced from the last session, and finally,
        :math:`\\vec{u}` is the unitary vector in the direction of the tube.



        :param ws: Integrated workspace with tubes to be calibrated.
        :param tubeSet: Specification of Set of tubes to be
        calibrated. If a string is passed, a TubeSpec will be created
        passing the string as the setTubeSpecByString.

        This will be the case for TubeSpec as string

        .. code-block:: python

            self.tube_spec = TubeSpec(ws)
            self.tube_spec.setTubeSpecByString(tubeSet)

        If a list of strings is passed, the TubeSpec will be created with this list:

        .. code-block:: python

            self.tube_spec = TubeSpec(ws)
            self.tube_spec.setTubeSpecByStringArray(tubeSet)

        If a :class:`~tube_spec.TubeSpec` object is passed, it will be used as it is.


        :param knownPositions: The defined position for the
        peaks/edges, taking the center as the origin and having the
        same units as the tube length in the 3D space.

        :param funcForm: list with special values to define the format
        of the peaks/edge (peaks=1, edge=2). If it is not provided, it
        will be assumed that all the knownPositions are peaks.


        Optionals parameters to tune the calibration:

        :param fitPar: Define the parameters to be used in the fit as
        a :class:`~tube_calib_fit_params.TubeCalibFitParams`. If not
        provided, the dynamic mode is used. See
        :py:func:`~Examples.TubeCalibDemoMaps_All.provideTheExpectedValue`

        :param margin: value in pixesl that will be used around the
        peaks/edges to fit them. Default = 15. See the code of
        :py:mod:`~Examples.TubeCalibDemoMerlin` where **margin** is
        used to calibrate small tubes.

        .. code-block:: python

            fit_start, fit_end = centre - margin, centre + margin

        :param rangeList: list of tubes indexes that will be
        calibrated. As in the following code (see:
        :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationSingleTube`):

        .. code-block:: python

            for index in rangelist:
                do_calibrate(tubeSet.getTube(index))

        :param calibTable: Pass the calibration table, it will them
        append the values to the provided one and return it. (see:
        :py:mod:`~Examples.TubeCalibDemoMerlin`)

        :param plotTube: If given, the tube whose index is in plotTube
        will be ploted as well as its fitted peaks, it can receive a
        list of indexes to plot.(see:
        :py:func:`~Examples.TubeCalibDemoMaps_All.changeMarginAndExpectedValue`)

        :param excludeShortTubes: Do not calibrate tubes whose length
        is smaller than given value. (see at:
        Examples/TubeCalibDemoMerlin_Adjustable.py)

        :param overridePeaks: dictionary that defines an array of
        peaks positions (in pixels) to be used for the specific
        tube(key). (see:
        :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationSingleTube`)

        .. code-block:: python

            for index in rangelist:
                if overridePeaks.has_key(index):
                use_this_peaks = overridePeaks[index]
                # skip finding peaks
                fit_peaks_to_position()

        :param fitPolyn: Define the order of the polinomial to fit the
        pixels positions agains the known positions. The acceptable
        values are 1, 2 or 3. Default = 2.


        :param outputPeak: Enable the calibrate to output the peak
        table, relating the tubes with the pixels positions. It may be
        passed as a boolean value (outputPeak=True) or as a peakTable
        value. The later case is to inform calibrate to append the new
        values to the given peakTable. This is usefull when you have
        to operate in subsets of tubes. (see
        :py:mod:`~Examples.TubeCalibDemoMerlin` that shows a nice
        inspection on this table).

        .. code-block:: python

            calibTable, peakTable = calibrate(ws, (omitted), rangeList=[1],
                    outputPeak=True)
            # appending the result to peakTable
            calibTable, peakTable = calibrate(ws, (omitted), rangeList=[2],
                    outputPeak=peakTable)
            # now, peakTable has information for tube[1] and tube[2]

        :rtype: calibrationTable, a TableWorkspace with two columns DetectorID(int) and DetectorPositions(V3D).

        """
        FITPAR = 'fitPar'
        MARGIN = 'margin'
        RANGELIST = 'rangeList'
        CALIBTABLE = 'calibTable'
        PLOTTUBE = 'plotTube'
        EXCLUDESHORT = 'excludeShortTubes'
        OVERRIDEPEAKS = 'overridePeaks'
        FITPOLIN = 'fitPolyn'
        OUTPUTPEAK = 'outputPeak'
        # RKH
        OUTPUTC = 'outputC'

        # check that only valid arguments were passed through kwargs
        for key in list(kwargs.keys()):
            if key not in [
                    FITPAR, MARGIN, RANGELIST, CALIBTABLE, PLOTTUBE,
                    EXCLUDESHORT, OVERRIDEPEAKS, FITPOLIN, OUTPUTPEAK, OUTPUTC
            ]:
                msg = ("Wrong argument: '%s'! This argument is not defined in the signature of this function. "
                       "Hint: remember that arguments are case sensitive" % key)
                raise RuntimeError(msg)

        # check parameter ws: if it was given as string, transform it in
        # mantid object
        if isinstance(ws, str):
            ws = mtd[ws]
        if not isinstance(ws, MatrixWorkspace):
            raise RuntimeError(
                "Wrong argument ws = %s. It must be a MatrixWorkspace" %
                (str(ws)))

        # check parameter tubeSet. It accepts string or preferable a TubeSpec
        if isinstance(tubeSet, str):
            selectedTubes = tubeSet
            tubeSet = TubeSpec(ws)
            tubeSet.setTubeSpecByString(selectedTubes)
        elif isinstance(tubeSet, list):
            selectedTubes = tubeSet
            tubeSet = TubeSpec(ws)
            tubeSet.setTubeSpecByStringArray(selectedTubes)
        elif not isinstance(tubeSet, TubeSpec):
            raise RuntimeError(
                "Wrong argument tubeSet. "
                "It must be a TubeSpec or a string that defines the set of tubes to be calibrated. For example: WISH/panel03"
            )

        # check the known_positions parameter
        # for old version compatibility, it also accepts IdealTube, eventhough
        # they should only be used internally
        if not (isinstance(knownPositions, list)
                or isinstance(knownPositions, tuple)
                or isinstance(knownPositions, np.ndarray)):
            raise RuntimeError(
                "Wrong argument knownPositions. "
                "It expects a list of values for the positions expected for the peaks in relation to the center of the tube"
            )
        else:
            idealTube = IdealTube()
            idealTube.setArray(np.array(knownPositions))

        # deal with funcForm parameter
        try:
            nPeaks = len(idealTube.getArray())
            if len(funcForm) != nPeaks:
                raise 1
                # RKH added options here
            for val in funcForm:
                if val not in [1, 2, 3]:
                    raise 2
        except:
            raise RuntimeError(
                ("Wrong argument FuncForm. "
                 "It expects a list of values describing the form of every single peaks. "
                 "So, for example, if there are three peaks where the first is a peak and the followers as edge, funcForm = [1, 2, 2]. "
                 "Currently, it is defined 1-Gaussian Peak, 2 - Edge. "
                 "The knownPos has %d elements and the given funcForm has %d."
                % (nPeaks, len(funcForm))))

        # apply the functional form to the ideal Tube
        idealTube.setForm(funcForm)

        # check the FITPAR parameter (optional)
        # if the FITPAR is given, than it will just pass on, if the FITPAR is
        # not given, it will create a FITPAR 'guessing' the centre positions,
        # and allowing the find peaks calibration methods to adjust the parameter
        # for the peaks automatically
        if FITPAR in kwargs:
            fitPar = kwargs[FITPAR]
            # fitPar must be a TubeCalibFitParams
            if not isinstance(fitPar, TubeCalibFitParams):
                raise RuntimeError(
                    "Wrong argument %s. This argument, when given, must be a valid TubeCalibFitParams object"
                    % FITPAR)
        else:
            # create a fit parameters guessing centre positions
            # the guessing obeys the following rule:
            #
            # centre_pixel = known_pos * ndets/tube_length + ndets / 2
            #
            # Get tube length and number of detectors
            tube_length = tubeSet.getTubeLength(0)
            # ndets = len(wsp_index_for_tube0)
            id1, ndets, step = tubeSet.getDetectorInfoFromTube(0)

            known_pos = idealTube.getArray()
            # position of the peaks in pixels
            centre_pixel = known_pos * ndets / tube_length + ndets * 0.5

            fitPar = TubeCalibFitParams(centre_pixel)
            # make it automatic, it means, that for every tube,
            # the parameters for fit will be re-evaluated, from the first
            # guess positions given by centre_pixel
            fitPar.setAutomatic(True)

        # check the MARGIN paramter (optional)
        if MARGIN in kwargs:
            try:
                margin = float(kwargs[MARGIN])
            except:
                raise RuntimeError(
                    "Wrong argument %s. It was expected a number!" % MARGIN)
            fitPar.setMargin(margin)

        # deal with RANGELIST parameter
        if RANGELIST in kwargs:
            rangeList = kwargs[RANGELIST]
            if isinstance(rangeList, int):
                rangeList = [rangeList]
            try:
                # this deals with list and tuples and iterables to make sure
                # rangeList becomes a list
                rangeList = list(rangeList)
            except:
                raise RuntimeError(
                    "Wrong argument %s. It expects a list of indexes for calibration"
                    % RANGELIST)
        else:
            rangeList = list(range(tubeSet.getNumTubes()))

        # check if the user passed the option calibTable
        if CALIBTABLE in kwargs:
            calibTable = kwargs[CALIBTABLE]
            # ensure the correct type is passed
            # if a string was passed, transform it in mantid object
            if isinstance(calibTable, str):
                calibTable = mtd[calibTable]
            # check that calibTable has the expected form
            try:
                if not isinstance(calibTable, ITableWorkspace):
                    raise 1
                if calibTable.columnCount() != 2:
                    raise 2
                colNames = calibTable.getColumnNames()
                if colNames[0] != 'Detector ID' or colNames[
                        1] != 'Detector Position':
                    raise 3
            except:
                raise RuntimeError(
                    "Invalid type for %s. The expected type was ITableWorkspace with 2 columns(Detector ID and Detector Positions)"
                    % CALIBTABLE)
        else:
            calibTable = CreateEmptyTableWorkspace(
                OutputWorkspace="CalibTable")
            # "Detector ID" column required by ApplyCalibration
            calibTable.addColumn(type="int", name="Detector ID")
            # "Detector Position" column required by ApplyCalibration
            calibTable.addColumn(type="V3D", name="Detector Position")

        # deal with plotTube option
        if PLOTTUBE in kwargs:
            plotTube = kwargs[PLOTTUBE]
            if isinstance(plotTube, int):
                plotTube = [plotTube]
            try:
                plotTube = list(plotTube)
            except:
                raise RuntimeError(
                    "Wrong argument %s. It expects an index (int) or a list of indexes"
                    % PLOTTUBE)
        else:
            plotTube = []

        # deal with minimun tubes sizes
        if EXCLUDESHORT in kwargs:
            excludeShortTubes = kwargs[EXCLUDESHORT]
            try:
                excludeShortTubes = float(excludeShortTubes)
            except:
                raise RuntimeError(
                    "Wrong argument %s. It expects a float value for the minimun size of tubes to be calibrated"
                )
        else:
            # a tube with length 0 can not be calibrated, this is the minimun value
            excludeShortTubes = 0.0

        # deal with OVERRIDEPEAKS parameters
        if OVERRIDEPEAKS in kwargs:
            overridePeaks = kwargs[OVERRIDEPEAKS]
            try:
                nPeaks = len(idealTube.getArray())
                # check the format of override peaks
                if not isinstance(overridePeaks, dict):
                    raise 1
                for key in list(overridePeaks.keys()):
                    if not isinstance(key, int):
                        raise 2
                    if key < 0 or key >= tubeSet.getNumTubes():
                        raise 3
                    if len(overridePeaks[key]) != nPeaks:
                        raise 4
            except:
                raise RuntimeError(
                    "Wrong argument %s. "
                    "It expects a dictionary with key as the tube index and the value as a list of peaks positions. "
                    "Ex (3 peaks): overridePeaks = {1:[2,5.4,500]}"
                    % OVERRIDEPEAKS)
        else:
            overridePeaks = dict()

        # deal with FITPOLIN parameter
        if FITPOLIN in kwargs:
            polinFit = kwargs[FITPOLIN]
            if polinFit not in [1, 2, 3]:
                raise RuntimeError(
                    "Wrong argument %s. It expects a number 1 for linear, "
                    "2 for quadratic, or 3 for 3rd polinomial order when fitting the pixels positions agains the known positions"
                    % FITPOLIN)
        else:
            polinFit = 2

        # deal with OUTPUT PEAK
        deletePeakTableAfter = False
        if OUTPUTPEAK in kwargs:
            outputPeak = kwargs[OUTPUTPEAK]
        else:
            outputPeak = False
        if isinstance(outputPeak, ITableWorkspace):
            if outputPeak.columnCount() < len(idealTube.getArray()):
                raise RuntimeError(
                    "Wrong argument %s. It expects a boolean flag, or a ITableWorksapce with columns (TubeId, Peak1,...,PeakM) "
                    "for M = number of peaks given in knownPositions"
                    % OUTPUTPEAK)
        else:
            if not outputPeak:
                deletePeakTableAfter = True
            # create the output peak table
            outputPeak = CreateEmptyTableWorkspace(OutputWorkspace="PeakTable")
            outputPeak.addColumn(type='str', name='TubeId')
            for i in range(len(idealTube.getArray())):
                outputPeak.addColumn(type='float', name='Peak%d' % (i + 1))

        # RKH added whole section here for outputC
        if OUTPUTC in kwargs:
            outputC = kwargs[OUTPUTC]
        else:
            outputC = False
        # RKH  added meanCTable
        outputC = CreateEmptyTableWorkspace(OutputWorkspace="meanCTable")
        outputC.addColumn(type='str', name='TubeId')
        outputC.addColumn(type='float', name='meanC')

        # RKH added meanCTable
        self.getCalibration_RKH(ws, tubeSet, calibTable, fitPar, idealTube,
                                outputPeak, outputC, overridePeaks,
                                excludeShortTubes, plotTube, rangeList,
                                polinFit)

        if deletePeakTableAfter:
            DeleteWorkspace(str(outputPeak))
            return calibTable
        else:
            # RKH added meanCTable
            return calibTable, outputPeak, outputC

    def savePeak(peakTable, filePath):
        """Allows to save the peakTable to a text file.

        :param peakTable: peak table as the workspace table provided by calibrated method, as in the example:

        .. code-block:: python

        calibTable, peakTable = calibrate(..., outputPeak=peakTable)
        savePeak(peakTable, 'myfolder/myfile.txt')

        :param filePath: where to save the file. If the filePath is
        not given as an absolute path, it will be considered relative
        to the defaultsave.directory.

        The file will be saved with the following format:

        id_name (parsed space to %20) [peak1, peak2, ..., peakN]

        You may load these peaks using readPeakFile

        ::

        panel1/tube001 [23.4, 212.5, 0.1]
        ...
        panel1/tubeN   [56.3, 87.5, 0.1]

        """
        if not os.path.isabs(filePath):
            saveDirectory = config['defaultsave.directory']
            pFile = open(os.path.join(saveDirectory, filePath), 'w')
        else:
            pFile = open(filePath, 'w')
        if isinstance(peakTable, str):
            peakTable = mtd[peakTable]
        nPeaks = peakTable.columnCount() - 1
        peaksNames = ['Peak%d' % (i + 1) for i in range(nPeaks)]

        for line in range(peakTable.rowCount()):
            row = peakTable.row(line)
            peak_values = [row[k] for k in peaksNames]
            tube_name = row['TubeId'].replace(' ', '%20')
            print(tube_name, peak_values, file=pFile)

        pFile.close()

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
        pattern = re.compile(r'[\[\],\s\r]')
        saveDirectory = config['defaultsave.directory']
        pfile = os.path.join(saveDirectory, file_name)
        for line in open(pfile, 'r'):
            # check if the entry is a comment line
            if line.startswith('#'):
                continue
                # split all values
            line_vals = re.split(pattern, line)
            id_ = str(line_vals[0]).replace('%20', ' ')
            if id_ == '':
                continue
            try:
                f_values = [float(v) for v in line_vals[1:] if v != '']
            except ValueError:
                # print 'Wrong format: we expected only numbers, but receive this line ',str(line_vals[1:])
                continue

            loaded_file.append((id_, f_values))
        return loaded_file

    def getCalibration_RKH(self,
                           ws,
                           tubeSet,
                           calibTable,
                           fitPar,
                           iTube,
                           peaksTable,
                           meanCTable,
                           overridePeaks=dict(),
                           excludeShortTubes=0.0,
                           plotTube=[],
                           rangeList=None,
                           polinFit=2,
                           peaksTestMode=False):
        """Get the results the calibration and put them in the calibration table provided.
        RKH added meanCTable to pass back mean of "resolution"
        :param ws: Integrated Workspace with tubes to be calibrated
        :param tubeSet: Specification of Set of tubes to be calibrated ( :class:`~tube_spec.TubeSpec` object)

        :param calibTable: Empty calibration table into which the
        calibration results are placed. It is composed by 'Detector
        ID' and a V3D column 'Detector Position'. It will be filled
        with the IDs and calibrated positions of the detectors.

        :param fitPar: A :class:`~tube_calib_fit_params.TubeCalibFitParams` object for fitting the peaks

        :param iTube: The :class:`~ideal_tube.IdealTube` which
        contains the positions in metres of the shadows of the slits,
        bars or edges used for calibration.

        :param peaksTable: Peaks table into wich the peaks positions will be put
        :param meanCTable: RKH added, mean value of "resolution" parameter

        :param overridePeak: dictionary with tube indexes keys and an
        array of peaks in pixels to override those that would be
        fitted for one tube

        :param exludeShortTubes: Exlude tubes shorter than specified length from calibration
        :param plotTube: List of tube indexes that will be ploted
        :param rangelist: list of the tube indexes that will be calibrated. Default None, means all the tubes in tubeSet
        :param polinFit: Order of the polinomial to fit against the known positions. Acceptable: 2, 3
        :param peakTestMode: true if shoving detectors that are reckoned to be at peak away (for test purposes)


        This is the main method called from :func:`~tube.calibrate` to perform the calibration.

        """
        nTubes = tubeSet.getNumTubes()
        self.log().debug(f"Number of tubes = {nTubes}")

        if rangeList is None:
            rangeList = list(range(nTubes))

        all_skipped = set()

        for i in rangeList:

            # Deal with (i+1)st tube specified
            wht, skipped = tubeSet.getTube(i)
            all_skipped.update(skipped)

            self.log().debug(f"Calibrating tube {i + 1} of {nTubes} {tubeSet.getTubeName(i)}")
            if (len(wht) < 1):
                self.log().debug(
                    "Unable to get any workspace indices (spectra) for this tube. "
                    f"Tube {tubeSet.getTubeName(i)} not calibrated.")
                # skip this tube
                continue

            # Calibribate the tube, if possible
            if (tubeSet.getTubeLength(i) <= excludeShortTubes):
                # skip this tube
                continue

            ##############################
            # Define Peak Position session
            ##############################

            # if this tube is to be override, get the peaks positions for this tube.
            if i in overridePeaks:
                actualTube = overridePeaks[i]
            else:
                # find the peaks positions
                plotThisTube = i in plotTube
                # RKH add meanC
                self.log().debug(str(iTube))
                self.log().debug(str(iTube.getFunctionalForms()))
                actualTube, meanC = self.getPoints(ws,
                                                   iTube.getFunctionalForms(),
                                                   fitPar,
                                                   wht,
                                                   showPlot=plotThisTube)
                if plotThisTube:
                    RenameWorkspace('FittedData',
                                    OutputWorkspace='FittedTube%d' % (i))
                    RenameWorkspace('TubePlot',
                                    OutputWorkspace='TubePlot%d' % (i))

            # Set the peak positions at the peakTable
            #  RKH add meanC to end
            # note for SANS2d we only call one tube at a time, as the
            # number of edges (peaks) varies tube to tube and thus addRow does not work!
            peaksTable.addRow([tubeSet.getTubeName(i)] + list(actualTube))
            self.log().debug(f"meanC {meanC}")
            meanCTable.addRow([tubeSet.getTubeName(i)] + list(meanC))

            ##########################################
            # Define the correct position of detectors
            ##########################################

            detIDList, detPosList = self.getCalibratedPixelPositions_RKH(
                ws, actualTube, iTube.getArray(), wht, peaksTestMode, polinFit)
            # save the detector positions to calibTable
            if (len(detIDList) == len(wht)):  # We have corrected positions
                for j in range(len(wht)):
                    nextRow = {
                        'Detector ID': detIDList[j],
                        'Detector Position': detPosList[j]
                    }
                    calibTable.addRow(nextRow)

        if len(all_skipped) > 0:
            self.log().debug(
                f"{len(all_skipped)} histogram(s) were excluded from the calibration since they did not have an assigned detector.")

    def createTubeCalibtationWorkspaceByWorkspaceIndexList(
            self,
            integratedWorkspace,
            outputWorkspace,
            workspaceIndexList,
            xUnit='Pixel',
            showPlot=False):
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
        if (nSpectra < 1):
            return
        pixelNumbers = []
        integratedPixelCounts = []
        pixel = 1
        # integratedWorkspace.
        for i in workspaceIndexList:
            pixelNumbers.append(pixel)
            pixel = pixel + 1
            integratedPixelCounts.append(integratedWorkspace.dataY(i)[0])

        CreateWorkspace(dataX=pixelNumbers,
                        dataY=integratedPixelCounts,
                        OutputWorkspace=outputWorkspace)
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
        idlow, low = self.get_detector_pos(
            work_handle, center_low_pixel
        )  # Get the detector position of the nearest lower pixel
        idhigh, high = self.get_detector_pos(
            work_handle, center_high_pixel
        )  # Get the detector position of the nearest higher pixel
        center_y = (center_high_pixel - pixel_float) * low.getY() + (
            pixel_float - center_low_pixel) * high.getY()
        center_y /= (center_high_pixel - center_low_pixel)
        return center_y

    def fitGaussianParams(self, height, centre,
                          sigma):  # Compose string argument for fit
        # print "name=Gaussian, Height="+str(height)+", PeakCentre="+str(centre)+", Sigma="+str(sigma)
        return "name=Gaussian, Height=" + str(height) + ", PeakCentre=" + str(
            centre) + ", Sigma=" + str(sigma)

    def fitEndErfcParams(self, B, C):  # Compose string argument for fit
        # print "name=EndErfc, B="+str(B)+", C="+str(C)
        return "name=EndErfc, B=" + str(B) + ", C=" + str(C)

    # RKH 11/11/19
    def fitFlatTopPeakParams(self, centre, endGrad,
                             width):  # Compose string argument for fit
        # print "name=EndErfc, B="+str(B)+", C="+str(C)
        return "name=FlatTopPeak, Centre=" + str(centre) + ", endGrad=" + str(
            endGrad) + ", Width=" + str(width)

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
        Fit(InputWorkspace=ws,
            Function=self.fitFlatTopPeakParams(centre, endGrad, width),
            StartX=str(start),
            EndX=str(end),
            Output=outputWs)
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
        values = all_values[max(int(centre - margin), 0
                                ):min(int(centre + margin), len(all_values))]

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
        Fit(InputWorkspace=ws,
            Function=fitEndErfcParams(centre, endGrad * edgeMode),
            StartX=str(start),
            EndX=str(end),
            Output=outputWs)
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

            fit_msg = 'name=LinearBackground,A0=%f;name=Gaussian,Height=%f,PeakCentre=%f,Sigma=%f' % (
                background, height, centre, width)

            Fit(InputWorkspace=ws,
                Function=fit_msg,
                StartX=str(start),
                EndX=str(end),
                Output=outputWs)

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
            Fit(InputWorkspace=ws,
                Function='name=LinearBackground,A0=%f' % (background),
                StartX=str(start),
                EndX=str(end),
                Output='Z1')
            Fit(InputWorkspace='Z1_Workspace',
                Function='name=Gaussian,Height=%f,PeakCentre=%f,Sigma=%f' %
                (height, centre, width),
                WorkspaceIndex=2,
                StartX=str(start),
                EndX=str(end),
                Output=outputWs)
            CloneWorkspace(outputWs + '_Workspace',
                           OutputWorkspace='gauss_' + str(index))
            peakIndex = 1

        return peakIndex

    def getPoints(self,
                  IntegratedWorkspace,
                  funcForms,
                  fitParams,
                  whichTube,
                  showPlot=False):
        """
        Get the centres of N slits or edges for calibration

        It does look for the peak position in pixels by fitting the peaks and
        edges. It is the method responsible for estimating the peak position in each tube.

        .. note::
        This N slit method is suited for WISH or the five sharp peaks of MERLIN .

        :param IntegratedWorkspace: Workspace of integrated data
        :param funcForms: array of function form 1=slit/bar (Gaussian peak), 2=edge, 3= FlatTopPeak (pair of edges that can partly overlap)
        :param fitParams: a TubeCalibFitParams object contain the fit parameters
        :param whichTube:  a list of workspace indices for one tube (define a single tube)
        :param showPlot: show plot for this tube

        :rtype: array of the slit/edge positions (-1.0 indicates failed to find position)

        """

        # Create input workspace for fitting
        ## get all the counts for the integrated workspace inside the tube
        countsY = np.array(
            [IntegratedWorkspace.dataY(i)[0] for i in whichTube])
        if (len(countsY) == 0):
            return
        getPointsWs = CreateWorkspace(list(range(len(countsY))),
                                      countsY,
                                      OutputWorkspace='TubePlot')
        calibPointWs = 'CalibPoint'
        results = []
        fitt_y_values = []
        fitt_x_values = []

        # RKH added funcForms 2, 3 (11/11/19)
        mean = 0.0
        meanC = []
        j = 0
        # Loop over the points
        temp = []
        self.log().debug(str(len(funcForms)))
        for i in range(len(funcForms)):
            self.log().debug(f"{i}")
            if funcForms[i] == 3:
                # find the FlatTopPeak position
                # RKH try save the fit params to get avg resolution
                peakIndex = self.fitFlatTopPeak(fitParams, i, getPointsWs,
                                                calibPointWs)

                cc = list(mtd[calibPointWs + '_Parameters'].row(2).items())[1][1]
                temp.append(cc)
                cc = np.fabs(cc)
                if cc > 1e-06:
                    j += 1
                    mean += cc
            elif funcForms[i] == 2:
                # find the edge position
                # RKH try save the fit params to get avg resolution
                peakIndex = fitEdges(fitParams, i, getPointsWs, calibPointWs)

                cc = list(mtd[calibPointWs + '_Parameters'].row(2).items())[1][1]
                temp.append(cc)
                cc = np.fabs(cc)
                if cc > 1e-06:
                    j += 1
                    mean += cc
            else:
                peakIndex = self.fitGaussian(fitParams, i, getPointsWs,
                                             calibPointWs)
            # get the peak centre
            peakCentre = list(mtd[calibPointWs + '_Parameters'].row(peakIndex).items())[1][1]
            results.append(peakCentre)
            #
            if showPlot:
                ws = mtd[calibPointWs + '_Workspace']
                fitt_y_values.append(copy.copy(ws.dataY(1)))
                fitt_x_values.append(copy.copy(ws.dataX(1)))
                self.log().debug(str(fitt_y_values))
                self.log().debug(str(fitt_x_values))

        # RKH
        if j > 0:
            mean = mean / float(j)
        #  meanC = CreateSingleValuedWorkspace(Datavalue=mean)  this did not work
        meanC.append(mean)
        # RKH
        self.log().debug(f"C values = {temp}")
        #
        if showPlot:
            FittedData = CreateWorkspace(np.hstack(fitt_x_values),
                                         np.hstack(fitt_y_values))
        # RKH return meanC also
        return results, meanC

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
            ideal.append(
                self.get_ypos(IntegratedWorkspace,
                              slits[i]))  # Use Pascal Manuel's Y conversion.

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
            xBinNew.append(
                xo + (xo * (nDets - xo) * GainError)
            )  # Final bin position values corrected for offsets and gain

        # print xBinNew
        return xBinNew

    def correctTubeToIdealTube(self,
                               tubePoints,
                               idealTubePoints,
                               nDets,
                               TestMode=False,
                               polinFit=2):
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
        if (len(tubePoints) != len(idealTubePoints)):
            self.log().debug(f"Number of points in tube {len(tubePoints)} must equal number of points in ideal tube {len(idealTubePoints)}")
            return xResult

        # Filter out rogue slit points
        usedTubePoints = []
        usedIdealTubePoints = []
        missedTubePoints = []  # Used for diagnostic print only
        for i in range(len(tubePoints)):
            if (tubePoints[i] > 0.0 and tubePoints[i] < nDets):
                usedTubePoints.append(tubePoints[i])
                usedIdealTubePoints.append(idealTubePoints[i])
            else:
                missedTubePoints.append(i + 1)

        # State number of rogue slit points, if any
        if (len(tubePoints) != len(usedTubePoints)):
            self.log().debug(f"Only {len(usedTubePoints)} out of {len(tubePoints)} slit points used. Missed {missedTubePoints}")

        # Check number of usable points
        if (len(usedTubePoints) < 3):
            self.log().debug(f"Too few usable points in tube {len(usedTubePoints)}")
            return []

        # Fit quadratic to ideal tube points
        CreateWorkspace(dataX=usedTubePoints,
                        dataY=usedIdealTubePoints,
                        OutputWorkspace="PolyFittingWorkspace")
        try:
            Fit(InputWorkspace="PolyFittingWorkspace",
                Function='name=Polynomial,n=%d' % (polinFit),
                StartX=str(0.0),
                EndX=str(nDets),
                Output="QF")
        except:
            self.log().debug("Fit failed")
            return []

        paramQF = mtd['QF_Parameters']

        # get the coeficients, get the Value from every row, and exclude the last one because it is the error
        # rowErr is the last one, it could be used to check accuracy of fit
        c = [r['Value'] for r in paramQF][:-1]

        # Modify the output array by the fitted quadratic
        xResult = np.polynomial.polynomial.polyval(list(range(nDets)), c)

        # In test mode, shove the pixels that are closest to the reckoned peaks
        # to the position of the first detector so that the resulting gaps can be seen.
        if (TestMode):
            self.log().debug("TestMode code")
            for i in range(len(usedTubePoints)):
                # print "used point",i,"shoving pixel",int(usedTubePoints[i])
                xResult[int(usedTubePoints[i])] = xResult[0]

        # print xResult
        return xResult

    def getCalibratedPixelPositions_RKH(self,
                                        ws,
                                        tubePts,
                                        idealTubePts,
                                        whichTube,
                                        peakTestMode=False,
                                        polinFit=2):
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
        if (nDets < 1):
            return detIDs, detPositions

            # Correct positions of detectors in tube by quadratic fit
        pixels = self.correctTubeToIdealTube(tubePts,
                                             idealTubePts,
                                             nDets,
                                             TestMode=peakTestMode,
                                             polinFit=polinFit)
        # print pixels
        if (len(pixels) != nDets):
            self.log().debug("Tube correction failed.")
            return detIDs, detPositions
        baseInstrument = ws.getInstrument().getBaseInstrument()
        # Get tube unit vector
        # get the detector from the baseInstrument, in order to get the positions
        # before any calibration being loaded.
        det0 = baseInstrument.getDetector(ws.getDetector(whichTube[0]).getID())
        detN = baseInstrument.getDetector(
            ws.getDetector(whichTube[-1]).getID())
        d0pos, dNpos = det0.getPos(), detN.getPos()
        ## identical to norm of vector: |dNpos - d0pos|
        tubeLength = det0.getDistance(detN)
        if (tubeLength <= 0.0):
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
            det_pos = deti.getPos()
            pNew = pixels[i]
            # again, the opeartion float * v3d is not defined, but v3d * float is,
            # so, I wrote the new pos as center + unit_vector * (float)
            newPos = center + unit_vector * pNew

            detIDs.append(deti.getID())
            detPositions.append(newPos)
            # print i, detIDs[i], detPositions[i]

        return detIDs, detPositions


# Register algorithm with Mantid
AlgorithmFactory.subscribe(Calibrate)
