# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

import numpy
import os
import re

from mantid.kernel import V3D
from mantid.api import MatrixWorkspace, ITableWorkspace
from mantid.simpleapi import mtd, CreateEmptyTableWorkspace, DeleteWorkspace, FindPeaks, config
from tube_spec import TubeSpec
from ideal_tube import IdealTube
from tube_calib_fit_params import TubeCalibFitParams
from tube_calib import getCalibration, getCalibratedPixelPositions, getPoints

# Need to avoid flake8 warning but we can't do that with this
# buried directly in the string
CALIBRATE_SIGNATURE = "ws, tubeSet, knownPositions, funcForm, [fitPar, margin, rangeList, calibTable, plotTube, excludeShorTubes, overridePeaks, fitPolyn, outputPeak]"  # noqa

__doc__ = _MODULE_DOC = """
=========================
Definition of Calibration
=========================

.. autofunction:: calibrate({0})

=========
Use Cases
=========

Among the examples, inside the :py:mod:`Examples` folder, the user is encouraged to look at
:py:mod:`~Examples.TubeCalibDemoMaps_All`, there he will find 7 examples showing how to use calibrate method.

* :py:func:`~Examples.TubeCalibDemoMaps_All.minimalInput` shows the easiest way to use calibrate.
* :py:func:`~Examples.TubeCalibDemoMaps_All.provideTheExpectedValue` shows the usage of **fitPar** parameter to provide
  the expected values for the peaks in pixels.
* :py:func:`~Examples.TubeCalibDemoMaps_All.changeMarginAndExpectedValue` demonstrate how to use **margin**, **fitPar**,
  **plotTube**, and **outputPeak**
* :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationSingleTube` explores the usage of **rangeList** and
  **overridePeaks** to improve the calibration of specific tubes.
* :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationOfListOfTubes` extends theimprovingCalibrationSingleTube
  to provide a good calibration to almost all instrument.
* :py:func:`~Examples.TubeCalibDemoMaps_All.calibrateB2Window` explore a singularity of the MAP14919 example, where the
  second peak does not appear clear on some tubes inside one door. So, this example, shows how to use **rangeList** to
  carry a calibration to the group of tubes.
* :py:func:`~Examples.TubeCalibDemoMaps_All.completeCalibration` demonstrate how the **rangeList**, **overridePeaks**,
  may be used together to allow the calibration of the whole instrument, despite, its particularities in some cases.
* :py:func:`~Examples.TubeCalibDemoMaps_All.findThoseTubesThatNeedSpecialCareForCalibration` show an approach to find the
  tubes that will require special care on calibrating. It will also help to find detectors that are not working well.

========
Examples
========

.. automodule:: Examples

====================
Other Useful Methods
====================

.. autofunction:: tube.savePeak

.. autofunction:: tube.readPeakFile

.. autofunction:: tube.saveCalibration

.. autofunction:: tube.readCalibrationFile

""".format(CALIBRATE_SIGNATURE)


def calibrate(ws, tubeSet, knownPositions, funcForm, **kwargs):
    """
    Define the calibrated positions of the detectors inside the tubes defined
    in tubeSet.

    Tubes may be considered a list of detectors aligned that may be considered
    as pixels for the analogy when they values are displayed.

    The position of these pixels are provided by the manufacturer, but its real
    position depends on the electronics inside the tube and varies slightly
    from tube to tube. The calibrate method, aims to find the real positions
    of the detectors (pixels) inside the tube.

    For this, it will receive an Integrated workspace, where a special
    measurement was performed so to have a
    pattern of peaks or through. Where gaussian peaks or edges can be found.


    The calibration follows the following steps

    1. Finding the peaks on each tube
    2. Fitting the peaks against the Known Positions
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

    **Dynamically fitting peaks**

    The framework expects that for each tube, it will find a peak pattern
    around the pixels corresponding to the known_pos positions.

    The way it will work out the estimated peak position (in pixel) is:

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

    **Force Fitting Parameters**

    These dynamically values can be avoided by defining the **fitPar** for
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

    **Different Function Factors**

    Although the examples consider only Gaussian peaks, it is possible to
    change the function factors to edges by passing the index of the
    known_position through the **funcForm**. Hence, considering three special
    points, where there are one gaussian peak and two edges, the calibrate
    could be configured as

    .. code-block:: python

       known_pos = [-0.1 2 2.3]
       # gaussian peak followed by two edges (through)
       form_factor = [1 2 2]
       calibTable = calibrate(ws,'WISH/panel03',known_pos, form_factor)


    **Override Peaks**

    It is possible to scape the finding peaks position steps by providing the
    peaks through the **overridePeaks** parameters. The example below tests
    the calibration of a single tube (30) but skips the finding peaks step.

    .. code-block:: python

       known_pos = [-0.41,-0.31,-0.21,-0.11,-0.02, 0.09, 0.18, 0.28, 0.39 ]
       define_peaks = [57.5, 107.0, 156.5, 206.0, 255.5, 305.0, 354.5,
                      404.0, 453.5]
       calibTable = calibrate(ws, 'WISH/panel03', known_pos, peaks_form,
                        overridePeaks={30:define_peaks}, rangeList=[30])

    **Output Peaks Positions**

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

    The signature changes to::

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

    The fitting framework of Mantid stores values and errors for the optimized coefficients of the polynomial
    in a table (of type TableWorkspace). These tables can be grouped into a WorkspaceGroup by passing
    the name of this workspace to option **parameters_table_group**. The name of each table workspace will
    the string **parameters_table_group** plus a suffix which is the index of the tube in the input **tubeSet**.

    **Define the new position for the detectors**

    Finally, the position of the detectors are defined as a vector operation
    like

    .. math::

      \\vec{p} = \\vec{c} + v \\vec{u}

    Where :math:`\\vec{p}` is the position in the 3D space, **v** is the
    RealRelativePosition deduced from the last session, and finally,
    :math:`\\vec{u}` is the unitary vector in the direction of the tube.



    :param ws: Integrated workspace with tubes to be calibrated.
    :param tubeSet: Specification of Set of tubes to be calibrated. If a string is passed, a TubeSpec will be created \
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


    :param knownPositions: The defined position for the peaks/edges, taking the center as the origin and having the \
    same units as the tube length in the 3D space.

    :param funcForm: list with special values to define the format of the peaks/edge (peaks=1, edge=2). If it is not \
    provided, it will be assumed that all the knownPositions are peaks.


    Optionals parameters to tune the calibration:

    :param fitPar: Define the parameters to be used in the fit as a :class:`~tube_calib_fit_params.TubeCalibFitParams`. \
    If not provided, the dynamic mode is used. See :py:func:`~Examples.TubeCalibDemoMaps_All.provideTheExpectedValue`

    :param margin: value in pixesl that will be used around the peaks/edges to fit them. Default = 15. See the code of \
    :py:mod:`~Examples.TubeCalibDemoMerlin` where **margin** is used to calibrate small tubes.

    .. code-block:: python

       fit_start, fit_end = centre - margin, centre + margin

    :param rangeList: list of tubes indexes that will be calibrated. As in the following code \
    (see: :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationSingleTube`):

    .. code-block:: python

       for index in rangelist:
           do_calibrate(tubeSet.getTube(index))

    :param calibTable: Pass the calibration table, it will them append the values to the provided one and return it. \
    (see: :py:mod:`~Examples.TubeCalibDemoMerlin`)

    :param plotTube: If given, the tube whose index is in plotTube will be ploted as well as its fitted peaks, it can \
    receive a list of indexes to plot.(see: :py:func:`~Examples.TubeCalibDemoMaps_All.changeMarginAndExpectedValue`)

    :param excludeShortTubes: Do not calibrate tubes whose length is smaller than given value. (see at \
    Examples/TubeCalibDemoMerlin_Adjustable.py)

    :param overridePeaks: dictionary that defines an array of peaks positions (in pixels) to be used for the specific \
    tube(key). (see: :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationSingleTube`)

    .. code-block:: python

       for index in rangelist:
         if overridePeaks.has_key(index):
           use_this_peaks = overridePeaks[index]
           # skip finding peaks
           fit_peaks_to_position()

    :param fitPolyn: Define the order of the polynomial to fit the pixels positions against the known positions. The \
    acceptable values are 1, 2 or 3. Default = 2.


    :param outputPeak: Enable the calibrate to output the peak table, relating the tubes with the pixels positions. It \
    may be passed as a boolean value (outputPeak=True) or as a peakTable value. The later case is to inform calibrate \
    to append the new values to the given peakTable. This is useful when you have to operate in subsets of tubes. \
    (see :py:mod:`~Examples.TubeCalibDemoMerlin` that shows a nice inspection on this table).

    .. code-block:: python

      calibTable, peakTable = calibrate(ws, (omitted), rangeList=[1],
               outputPeak=True)
      # appending the result to peakTable
      calibTable, peakTable = calibrate(ws, (omitted), rangeList=[2],
               outputPeak=peakTable)
      # now, peakTable has information for tube[1] and tube[2]

    :rtype: calibrationTable, a TableWorkspace with two columns DetectorID(int) and DetectorPositions(V3D).

    """
    # Legacy code requires kwargs to contain only the list of parameters specify below. Thus, we pop other
    # arguments into temporary variables, such as `parameters_table_group`
    parameters_table_group = kwargs.pop("parameters_table_group") if "parameters_table_group" in kwargs else None

    FITPAR = "fitPar"
    MARGIN = "margin"
    RANGELIST = "rangeList"
    CALIBTABLE = "calibTable"
    PLOTTUBE = "plotTube"
    EXCLUDESHORT = "excludeShortTubes"
    OVERRIDEPEAKS = "overridePeaks"
    FITPOLIN = "fitPolyn"
    OUTPUTPEAK = "outputPeak"

    param_helper = _CalibrationParameterHelper(
        FITPAR, MARGIN, RANGELIST, CALIBTABLE, PLOTTUBE, EXCLUDESHORT, OVERRIDEPEAKS, FITPOLIN, OUTPUTPEAK
    )

    # check that only valid arguments were passed through kwargs
    param_helper.ensure_no_unknown_kwargs(kwargs)

    # check parameter ws: if it was given as string, transform it in
    # mantid object
    ws = _CalibrationParameterHelper.enforce_matrix_ws(ws)

    # check parameter tubeSet. It accepts string or preferable a TubeSpec
    tubeSet = _CalibrationParameterHelper.enforce_tube_spec(tubeSet, ws)

    # check the known_positions parameter
    # for old version compatibility, it also accepts IdealTube, even though
    # they should only be used internally
    _CalibrationParameterHelper.validate_known_positions(knownPositions)
    ideal_tube = IdealTube()
    ideal_tube.setArray(numpy.array(knownPositions))

    n_peaks = len(ideal_tube.getArray())
    # deal with funcForm parameter
    _CalibrationParameterHelper.validate_func_form(funcForm, n_peaks)

    # apply the functional form to the ideal Tube
    ideal_tube.setForm(funcForm)

    # check the FITPAR parameter (optional)
    # if the FITPAR is given, than it will just pass on, if the FITPAR is
    # not given, it will create a FITPAR 'guessing' the centre positions,
    # and allowing the find peaks calibration methods to adjust the parameter
    # for the peaks automatically
    fit_par = param_helper.get_parameter(FITPAR, kwargs, tube_set=tubeSet, ideal_tube=ideal_tube)

    if MARGIN in kwargs:
        margin = param_helper.get_parameter(MARGIN, kwargs)
        fit_par.setMargin(margin)

    range_list = param_helper.get_parameter(RANGELIST, kwargs, default_range_list=list(range(tubeSet.getNumTubes())))
    calib_table = param_helper.get_parameter(CALIBTABLE, kwargs)
    plot_tube = param_helper.get_parameter(PLOTTUBE, kwargs)
    exclude_short_tubes = param_helper.get_parameter(EXCLUDESHORT, kwargs)
    override_peaks = param_helper.get_parameter(OVERRIDEPEAKS, kwargs, tube_set=tubeSet, ideal_tube=ideal_tube)
    polin_fit = param_helper.get_parameter(FITPOLIN, kwargs)  # order of the fiting polynomial. Default is 2
    output_peak, delete_peak_table_after = param_helper.get_parameter(OUTPUTPEAK, kwargs, ideal_tube=ideal_tube)

    getCalibration(
        ws,
        tubeSet,
        calib_table,
        fit_par,
        ideal_tube,
        output_peak,
        override_peaks,
        exclude_short_tubes,
        plot_tube,
        range_list,
        polin_fit,
        parameters_table_group=parameters_table_group,
    )

    if delete_peak_table_after:
        DeleteWorkspace(str(output_peak))
        return calib_table
    else:
        return calib_table, output_peak


def savePeak(peakTable, filePath):
    """
    Allows to save the peakTable to a text file.

    :param peakTable: peak table as the workspace table provided by calibrated method, as in the example:

    .. code-block:: python

       calibTable, peakTable = calibrate(..., outputPeak=peakTable)
       savePeak(peakTable, 'myfolder/myfile.txt')

    :param filePath: where to save the file. If the filePath is not given as an absolute path, it will be considered
                     relative to the defaultsave.directory.

    The file will be saved with the following format:

    id_name (parsed space to %20) [peak1, peak2, ..., peakN]

    You may load these peaks using readPeakFile

    ::

      panel1/tube001 [23.4, 212.5, 0.1]
      ...
      panel1/tubeN   [56.3, 87.5, 0.1]

    """
    if not os.path.isabs(filePath):
        saveDirectory = config["defaultsave.directory"]
        pFile = open(os.path.join(saveDirectory, filePath), "w")
    else:
        pFile = open(filePath, "w")
    if isinstance(peakTable, str):
        peakTable = mtd[peakTable]
    nPeaks = peakTable.columnCount() - 1
    peaksNames = ["Peak%d" % (i + 1) for i in range(nPeaks)]

    for line in range(peakTable.rowCount()):
        row = peakTable.row(line)
        peak_values = [row[k] for k in peaksNames]
        tube_name = row["TubeId"].replace(" ", "%20")
        print(tube_name, peak_values, file=pFile)

    pFile.close()


def readPeakFile(file_name):
    """Load the file calibration

    It returns a list of tuples, where the first value is the detector identification
    and the second value is its calibration values.

    Example of usage:

    .. code-block:: python

       for (det_code, cal_values) in readPeakFile('pathname/TubeDemo'):
          print(det_code)
          print(cal_values)

    :param file_name: Path for the file
    :rtype: list of tuples(det_code, peaks_values)

    """
    loaded_file = []
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
            continue

        loaded_file.append((id_, f_values))
    return loaded_file


def saveCalibration(table_name, out_path):
    """Save the calibration table to file

    This creates a CSV file for the calibration TableWorkspace. The first
    column is the detector number and the second column is the detector position

    Example of usage:

    .. code-block:: python

       saveCalibration('CalibTable','/tmp/myCalibTable.txt')

    :param table_name: name of the TableWorkspace to save
    :param out_path: location to save the file

    """
    DET = "Detector ID"
    POS = "Detector Position"
    with open(out_path, "w") as file_p:
        table = mtd[table_name]
        for row in table:
            row_data = [row[DET], row[POS]]
            line = ",".join(map(str, row_data)) + "\n"
            file_p.write(line)


def readCalibrationFile(table_name, in_path):
    """Read a calibration table from file

    This loads a calibration TableWorkspace from a CSV file.

    Example of usage:

    .. code-block:: python

       saveCalibration('CalibTable','/tmp/myCalibTable.txt')

    :param table_name: name to call the TableWorkspace
    :param in_path: the path to the calibration file

    """
    DET = "Detector ID"
    POS = "Detector Position"
    re_float = re.compile(r"[+-]? *(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?")
    calibTable = CreateEmptyTableWorkspace(OutputWorkspace=table_name)
    calibTable.addColumn(type="int", name=DET)
    calibTable.addColumn(type="V3D", name=POS)

    with open(in_path, "r") as file_p:
        for line in file_p:
            values = re.findall(re_float, line)
            if len(values) != 4:
                continue

            nextRow = {DET: int(values[0]), POS: V3D(float(values[1]), float(values[2]), float(values[3]))}

            calibTable.addRow(nextRow)


def findBadPeakFits(peaksTable, threshold=10):
    """Find peaks whose fit values fall outside of a given tolerance
    of the mean peak centers across all tubes.

    Tubes are defined as have a bad fit if the absolute difference
    between the fitted peak centers for a specific tube and the
    mean of the fitted peak centers for all tubes differ more than
    the threshold parameter.

    @param peakTable: the table containing fitted peak centers
    @param threshold: the tolerance on the difference from the mean value
    @return A list of expected peak positions and a list of indices of tubes
    to correct
    """
    n = len(peaksTable)
    num_peaks = peaksTable.columnCount() - 1
    column_names = ["Peak%d" % (i) for i in range(1, num_peaks + 1)]
    data = numpy.zeros((n, num_peaks))
    for i, row in enumerate(peaksTable):
        data_row = [row[name] for name in column_names]
        data[i, :] = data_row

    # data now has all the peaks positions for each tube
    # the mean value is the expected value for the peak position for each tube
    expected_peak_pos = numpy.mean(data, axis=0)

    # calculate how far from the expected position each peak position is
    distance_from_expected = numpy.abs(data - expected_peak_pos)
    check = numpy.where(distance_from_expected > threshold)[0]
    problematic_tubes = list(set(check))
    return expected_peak_pos, problematic_tubes


def cleanUpFit():
    """Clean up workspaces created by calibration fitting"""
    for ws_name in ("TubePlot", "RefittedPeaks", "PolyFittingWorkspace", "QF_NormalisedCovarianceMatrix", "QF_Parameters", "QF_Workspace"):
        try:
            DeleteWorkspace(ws_name)
        except:
            pass


def correctMisalignedTubes(ws, calibrationTable, peaksTable, spec, idealTube, fitPar, threshold=10):
    """Correct misaligned tubes due to poor fitting results
    during the first round of calibration.

    Misaligned tubes are first identified according to a tolerance
    applied to the absolute difference between the fitted tube
    positions and the mean across all tubes.

    The FindPeaks algorithm is then used to find a better fit
    with the ideal tube positions as starting parameters
    for the peak centers.

    From the refitted peaks the positions of the detectors in the
    tube are recalculated.

    @param ws: the workspace to get the tube geometry from
    @param calibrationTable: the calibration table output from running
    calibration
    @param peaksTable: the table containing the fitted peak centers from
    calibration
    @param spec: the tube spec for the instrument
    @param idealTube: the ideal tube for the instrument
    @param fitPar: the fitting parameters for calibration
    @param threshold: tolerance defining is a peak is outside of the acceptable
    range
    @return table of corrected detector positions
    """
    table_name = calibrationTable.name() + "Corrected"
    corrections_table = CreateEmptyTableWorkspace(OutputWorkspace=table_name)
    corrections_table.addColumn("int", "Detector ID")
    corrections_table.addColumn("V3D", "Detector Position")

    mean_peaks, bad_tubes = findBadPeakFits(peaksTable, threshold)

    for index in bad_tubes:
        print("Refitting tube %s" % spec.getTubeName(index))
        tube_dets, _ = spec.getTube(index)
        getPoints(ws, idealTube.getFunctionalForms(), fitPar, tube_dets)
        tube_ws = mtd["TubePlot"]
        fit_ws = FindPeaks(InputWorkspace=tube_ws, WorkspaceIndex=0, PeakPositions=fitPar.getPeaks(), PeaksList="RefittedPeaks")
        centers = [row["centre"] for row in fit_ws]
        detIDList, detPosList = getCalibratedPixelPositions(ws, centers, idealTube.getArray(), tube_dets)

        for id, pos in zip(detIDList, detPosList):
            corrections_table.addRow({"Detector ID": id, "Detector Position": V3D(*pos)})

        cleanUpFit()

    return corrections_table


class _CalibrationParameterHelper(object):
    def __init__(self, FITPAR, MARGIN, RANGELIST, CALIBTABLE, PLOTTUBE, EXCLUDESHORT, OVERRIDEPEAKS, FITPOLIN, OUTPUTPEAK):
        self.FITPAR = FITPAR
        self.MARGIN = MARGIN
        self.RANGELIST = RANGELIST
        self.CALIBTABLE = CALIBTABLE
        self.PLOTTUBE = PLOTTUBE
        self.EXCLUDESHORT = EXCLUDESHORT
        self.OVERRIDEPEAKS = OVERRIDEPEAKS
        self.FITPOLIN = FITPOLIN
        self.OUTPUTPEAK = OUTPUTPEAK
        self.allowed_kwargs = {FITPAR, MARGIN, RANGELIST, CALIBTABLE, PLOTTUBE, EXCLUDESHORT, OVERRIDEPEAKS, FITPOLIN, OUTPUTPEAK}

    def ensure_no_unknown_kwargs(self, kwargs):
        for key in kwargs.keys():
            if key not in self.allowed_kwargs:
                msg = (
                    "Wrong argument: '{0}'! This argument is not defined in the signature of this function. Hint: remember"
                    "that arguments are case sensitive".format(key)
                )
                raise RuntimeError(msg)

    @staticmethod
    def enforce_matrix_ws(ws):
        if isinstance(ws, MatrixWorkspace):
            return ws
        if isinstance(ws, str):
            return mtd[ws]
        raise RuntimeError("Wrong argument ws = %s. It must be a MatrixWorkspace" % (str(ws)))

    @staticmethod
    def enforce_tube_spec(tube_set, ws):
        if isinstance(tube_set, str):
            selected_tubes = tube_set
            tube_set = TubeSpec(ws)
            tube_set.setTubeSpecByString(selected_tubes)
            return tube_set
        if isinstance(tube_set, list):
            selected_tubes = tube_set
            tube_set = TubeSpec(ws)
            tube_set.setTubeSpecByStringArray(selected_tubes)
            return tube_set
        if isinstance(tube_set, TubeSpec):
            return tube_set
        raise RuntimeError(
            "Wrong argument tubeSet. "
            "It must be a TubeSpec or a string that defines the set of tubes to be calibrated. "
            "For example: WISH/panel03"
        )

    @staticmethod
    def validate_known_positions(known_positions):
        if not (isinstance(known_positions, list) or isinstance(known_positions, tuple) or isinstance(known_positions, numpy.ndarray)):
            raise RuntimeError(
                "Wrong argument knownPositions. It expects a list of values for the positions expected for the peaks in"
                "relation to the center of the tube"
            )

        for val in known_positions:
            if val >= 100:
                # Tube is greater than 100m - this is probably wrong so print an error
                raise ValueError(
                    "The following value: " + str(val) + " is greater or equal than 100m in length"
                    "\nHave you remembered to convert to meters?"
                )

    @staticmethod
    def validate_func_form(func_form, n_peaks):
        try:
            if len(func_form) != n_peaks:
                raise 1
            for val in func_form:
                if val not in [1, 2]:
                    raise 2
        except:
            raise RuntimeError(
                "Wrong argument FuncForm. It expects a list of values describing the form of everysingle peaks. So, for"
                "example, if there are three peaks where the first is a peak and the followers as edge,"
                "funcForm = [1, 2, 2].Currently, it is defined 1-Gaussian Peak, 2 - Edge. The knownPos has %d elements"
                "and the given funcForm has %d." % (n_peaks, len(func_form))
            )

    def get_parameter(self, name, args, **kwargs):
        if name == self.FITPAR:
            return self._get_fit_par(args, tube_set=kwargs["tube_set"], ideal_tube=kwargs["ideal_tube"])
        if name == self.MARGIN:
            return self._get_margin(args)
        if name == self.RANGELIST:
            return self._get_range_list(args, default_range_list=kwargs["default_range_list"])
        if name == self.CALIBTABLE:
            return self._get_calib_table(args)
        if name == self.PLOTTUBE:
            return self._get_plot_tube(args)
        if name == self.EXCLUDESHORT:
            return self._get_exclude_short(args)
        if name == self.OVERRIDEPEAKS:
            return self._get_override_peaks(args, tube_set=kwargs["tube_set"], ideal_tube=kwargs["ideal_tube"])
        if name == self.OUTPUTPEAK:
            return self._get_output_peak(args, ideal_tube=kwargs["ideal_tube"])
        if name == self.FITPOLIN:
            return self._get_fit_polin(args)

    def _get_output_peak(self, args, ideal_tube):
        delete_peak_table_after = False
        if self.OUTPUTPEAK in args:
            output_peak = args[self.OUTPUTPEAK]
        else:
            output_peak = False

        if isinstance(output_peak, ITableWorkspace):
            if output_peak.columnCount() < len(ideal_tube.getArray()):
                raise RuntimeError(
                    "Wrong argument {0}. "
                    "It expects a boolean flag, or a ITableWorksapce with columns (TubeId, Peak1,...,"
                    "PeakM) for M = number of peaks given in knownPositions".format(self.OUTPUTPEAK)
                )
            return output_peak, delete_peak_table_after

        else:
            if not output_peak:
                delete_peak_table_after = True

            # create the output peak table
            output_peak = CreateEmptyTableWorkspace(OutputWorkspace="PeakTable")
            output_peak.addColumn(type="str", name="TubeId")
            for i in range(len(ideal_tube.getArray())):
                output_peak.addColumn(type="float", name="Peak%d" % (i + 1))
            return output_peak, delete_peak_table_after

    def _get_fit_polin(self, args):
        r"""Order of the polynomial used to fit the observed peaks against known positions"""
        if self.FITPOLIN in args:
            polin_fit = args[self.FITPOLIN]
            if polin_fit not in [1, 2, 3]:
                raise RuntimeError(
                    "Wrong argument {0}. "
                    "It expects a number 1 for linear, 2 for quadratic, or 3 for 3rd polinomial order"
                    "when fitting the pixels positions against the known positions".format(self.FITPOLIN)
                )
            else:
                return polin_fit
        else:
            return 2

    def _get_override_peaks(self, args, tube_set, ideal_tube):
        if self.OVERRIDEPEAKS in args:
            override_peaks = args[self.OVERRIDEPEAKS]
            try:
                n_peaks = len(ideal_tube.getArray())
                # check the format of override peaks
                if not isinstance(override_peaks, dict):
                    raise 1
                for key in override_peaks.keys():
                    if not isinstance(key, int):
                        raise 2
                    if key < 0 or key >= tube_set.getNumTubes():
                        raise 3
                    if len(override_peaks[key]) != n_peaks:
                        raise 4
            except:
                raise RuntimeError(
                    f"Wrong argument {self.OVERRIDEPEAKS}. "
                    "It expects a dictionary with key as the tube index and the value as a list of peaks positions. "
                    "Ex (3 peaks): override_peaks = {1:[2,5.4,500]}"
                )
            else:
                return override_peaks
        else:
            return dict()

    def _get_exclude_short(self, args):
        if self.EXCLUDESHORT in args:
            exclude_short_tubes = args[self.EXCLUDESHORT]
            try:
                exclude_short_tubes = float(exclude_short_tubes)
            except:
                raise RuntimeError(
                    "Wrong argument {0}. It expects a float value for the minimum size of tubes to be calibrated".format(self.EXCLUDESHORT)
                )
            else:
                return exclude_short_tubes
        else:
            # a tube with length 0 can not be calibrated, this is the minimum value
            return 0.0

    def _get_plot_tube(self, args):
        if self.PLOTTUBE in args:
            plot_tube = args[self.PLOTTUBE]
            if isinstance(plot_tube, int):
                plot_tube = [plot_tube]
            try:
                plot_tube = list(plot_tube)
            except:
                raise RuntimeError("Wrong argument {0}. It expects an index (int) or a list of indexes".format(self.PLOTTUBE))
            else:
                return plot_tube
        else:
            return []

    def _get_calib_table(self, args):
        if self.CALIBTABLE in args:
            calib_table = args[self.CALIBTABLE]
            # ensure the correct type is passed
            # if a string was passed, transform it in mantid object
            if isinstance(calib_table, str):
                calib_table = mtd[calib_table]
            # check that calibTable has the expected form
            try:
                if not isinstance(calib_table, ITableWorkspace):
                    raise 1
                if calib_table.columnCount() != 2:
                    raise 2
                colNames = calib_table.getColumnNames()
                if colNames[0] != "Detector ID" or colNames[1] != "Detector Position":
                    raise 3
            except:
                raise RuntimeError(
                    "Invalid type for {0}."
                    "The expected type was ITableWorkspace with 2 columns(Detector ID and Detector Positions)".format(self.CALIBTABLE)
                )
            else:
                return calib_table
        else:
            calib_table = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
            # "Detector ID" column required by ApplyCalibration
            calib_table.addColumn(type="int", name="Detector ID")
            # "Detector Position" column required by ApplyCalibration
            calib_table.addColumn(type="V3D", name="Detector Position")
            return calib_table

    def _get_range_list(self, args, default_range_list):
        if self.RANGELIST in args:
            range_list = args[self.RANGELIST]
            if isinstance(range_list, int):
                range_list = range_list
            try:
                # this deals with list and tuples and iterables to make sure
                # range_list becomes a list
                range_list = list(range_list)
            except:
                raise RuntimeError("Wrong argument {0}. It expects a list of indexes for calibration".format(self.RANGELIST))
            else:
                return range_list
        else:
            return default_range_list

    def _get_margin(self, args):
        try:
            return float(args[self.MARGIN])
        except:
            raise RuntimeError("Wrong argument {0}. It was expected a number!".format(self.MARGIN))

    def _get_fit_par(self, args, tube_set, ideal_tube):
        if self.FITPAR in args:
            fit_par = args[self.FITPAR]
            try:
                assert getattr(fit_par, "setAutomatic")  # duck typing check for correct type
            except AssertionError:
                raise RuntimeError(
                    "Wrong argument {0}. This argument, when given, must be a valid TubeCalibFitParams object".format(self.FITPAR)
                )
        else:
            # create a fit parameters guessing centre positions
            # the guessing obeys the following rule:
            #
            # centre_pixel = known_pos * ndets/tube_length + ndets / 2
            #
            # Get tube length and number of detectors
            tube_length = tube_set.getTubeLength(0)
            # ndets = len(wsp_index_for_tube0)
            dummy_id1, ndets, dummy_step = tube_set.getDetectorInfoFromTube(0)

            known_pos = ideal_tube.getArray()
            # position of the peaks in pixels
            centre_pixel = known_pos * ndets / tube_length + ndets * 0.5

            fit_par = TubeCalibFitParams(centre_pixel)
            # make it automatic, it means, that for every tube,
            # the parameters for fit will be re-evaluated, from the first
            # guess positions given by centre_pixel
            fit_par.setAutomatic(True)

        return fit_par
