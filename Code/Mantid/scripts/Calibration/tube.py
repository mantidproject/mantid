"""
=========================
Definition of Calibration
=========================

.. autofunction:: calibrate(ws, tubeSet, knownPositions, funcForm, [fitPar, margin, rangeList, calibTable, plotTube, excludeShorTubes, overridePeaks, fitPolyn, outputPeak ] )

=========
Use Cases
=========

Among the examples, inside the :py:mod:`Examples` folder, the user is encouraged to look at
:py:mod:`~Examples.TubeCalibDemoMaps_All`, there he will find 7 examples showing how to use calibrate method.

* :py:func:`~Examples.TubeCalibDemoMaps_All.minimalInput` shows the easiest way to use calibrate.
* :py:func:`~Examples.TubeCalibDemoMaps_All.provideTheExpectedValue` shows the usage of **fitPar** parameter to provide the expected values for the peaks in pixels.
* :py:func:`~Examples.TubeCalibDemoMaps_All.changeMarginAndExpectedValue` demonstrate how to use **margin**, **fitPar**, **plotTube**, and **outputPeak**
* :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationSingleTube` explores the usage of **rangeList** and **overridePeaks** to improve the calibration of specific tubes.
* :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationOfListOfTubes` extends the improvingCalibrationSingleTube to provide a good calibration to almost all instrument.
* :py:func:`~Examples.TubeCalibDemoMaps_All.calibrateB2Window` explore a singularity of the MAP14919 example, where the second peak does not appear clear on some tubes inside one door. So, this example, shows how to use **rangeList** to carry a calibration to the group of tubes.
* :py:func:`~Examples.TubeCalibDemoMaps_All.completeCalibration` demonstrate how the **rangeList**, **overridePeaks**, may be used together to allow the calibration of the whole instrument, despite, its particularities in some cases.
* :py:func:`~Examples.TubeCalibDemoMaps_All.findThoseTubesThatNeedSpecialCareForCalibration` show an aproach to find the tubes that will require special care on calibrating. It will also help to find detectors that are not working well.

========
Examples
========

.. automodule:: Examples

=====================
Other Usefull Methods
=====================

.. autofunction:: tube.savePeak

.. autofunction:: tube.readPeakFile

"""

import numpy
import os
import re

from mantid.api import (MatrixWorkspace, ITableWorkspace)
from mantid.simpleapi import (mtd, CreateEmptyTableWorkspace, DeleteWorkspace, config)
from tube_spec import TubeSpec
from ideal_tube import IdealTube
from tube_calib_fit_params import TubeCalibFitParams
from tube_calib import getCalibration



def calibrate(ws, tubeSet, knownPositions, funcForm, **kwargs):
    """

      Define the calibrated positions of the detectors inside the tubes defined
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
      :param tubeSet: Specification of Set of tubes to be calibrated. If a string is passed, a TubeSpec will be created passing the string as the setTubeSpecByString.

       This will be the case for TubeSpec as string

       .. code-block:: python

         self.tube_spec = TubeSpec(ws)
         self.tube_spec.setTubeSpecByString(tubeSet)

       If a list of strings is passed, the TubeSpec will be created with this list:

       .. code-block:: python

          self.tube_spec = TubeSpec(ws)
          self.tube_spec.setTubeSpecByStringArray(tubeSet)

       If a :class:`~tube_spec.TubeSpec` object is passed, it will be used as it is.


      :param knownPositions: The defined position for the peaks/edges, taking the center as the origin and having the same units as the tube length in the 3D space.

      :param funcForm: list with special values to define the format of the peaks/edge (peaks=1, edge=2). If it is not provided, it will be assumed that all the knownPositions are peaks.


      Optionals parameters to tune the calibration:

      :param fitPar: Define the parameters to be used in the fit as a :class:`~tube_calib_fit_params.TubeCalibFitParams`. If not provided, the dynamic mode is used. See :py:func:`~Examples.TubeCalibDemoMaps_All.provideTheExpectedValue`

      :param margin: value in pixesl that will be used around the peaks/edges to fit them. Default = 15. See the code of :py:mod:`~Examples.TubeCalibDemoMerlin` where **margin** is used to calibrate small tubes.

       .. code-block:: python

          fit_start, fit_end = centre - margin, centre + margin

      :param rangeList: list of tubes indexes that will be calibrated. As in the following code (see: :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationSingleTube`):

       .. code-block:: python

          for index in rangelist:
              do_calibrate(tubeSet.getTube(index))

      :param calibTable: Pass the calibration table, it will them append the values to the provided one and return it. (see: :py:mod:`~Examples.TubeCalibDemoMerlin`)

      :param plotTube: If given, the tube whose index is in plotTube will be ploted as well as its fitted peaks, it can receive a list of indexes to plot.(see: :py:func:`~Examples.TubeCalibDemoMaps_All.changeMarginAndExpectedValue`)

      :param excludeShortTubes: Do not calibrate tubes whose length is smaller than given value. (see at: Examples/TubeCalibDemoMerlin_Adjustable.py)

      :param overridePeaks: dictionary that defines an array of peaks positions (in pixels) to be used for the specific tube(key). (see: :py:func:`~Examples.TubeCalibDemoMaps_All.improvingCalibrationSingleTube`)

       .. code-block:: python

          for index in rangelist:
            if overridePeaks.has_key(index):
              use_this_peaks = overridePeaks[index]
              # skip finding peaks
              fit_peaks_to_position()

      :param fitPolyn: Define the order of the polinomial to fit the pixels positions agains the known positions. The acceptable values are 1, 2 or 3. Default = 2.


      :param outputPeak: Enable the calibrate to output the peak table, relating the tubes with the pixels positions. It may be passed as a boolean value (outputPeak=True) or as a peakTable value. The later case is to inform calibrate to append the new values to the given peakTable. This is usefull when you have to operate in subsets of tubes. (see :py:mod:`~Examples.TubeCalibDemoMerlin` that shows a nice inspection on this table).

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

    #check that only valid arguments were passed through kwargs
    for key in kwargs.keys():
        if key not in [FITPAR, MARGIN, RANGELIST, CALIBTABLE, PLOTTUBE,
                       EXCLUDESHORT, OVERRIDEPEAKS, FITPOLIN,
                       OUTPUTPEAK]:
            msg = "Wrong argument: '%s'! This argument is not defined in the signature of this function. Hint: remember that arguments are case sensitive" % key
            raise RuntimeError(msg)


    # check parameter ws: if it was given as string, transform it in
    # mantid object
    if isinstance(ws,str):
        ws = mtd[ws]
    if not isinstance(ws,MatrixWorkspace):
        raise RuntimeError("Wrong argument ws = %s. It must be a MatrixWorkspace" % (str(ws)))

    # check parameter tubeSet. It accepts string or preferable a TubeSpec
    if isinstance(tubeSet,str):
        selectedTubes = tubeSet
        tubeSet = TubeSpec(ws)
        tubeSet.setTubeSpecByString(selectedTubes)
    elif isinstance(tubeSet, list):
        selectedTubes = tubeSet
        tubeSet = TubeSpec(ws)
        tubeSet.setTubeSpecByStringArray(selectedTubes)
    elif not isinstance(tubeSet,TubeSpec):
        raise RuntimeError("Wrong argument tubeSet. It must be a TubeSpec or a string that defines the set of tubes to be calibrated. For example: WISH/panel03")

    # check the known_positions parameter
    # for old version compatibility, it also accepts IdealTube, eventhough
    # they should only be used internally
    if not (isinstance(knownPositions, list) or
            isinstance(knownPositions, tuple) or
            isinstance(knownPositions, numpy.ndarray)):
        raise RuntimeError("Wrong argument knownPositions. It expects a list of values for the positions expected for the peaks in relation to the center of the tube")
    else:
        idealTube = IdealTube()
        idealTube.setArray(numpy.array(knownPositions))


    #deal with funcForm parameter
    try:
        nPeaks = len(idealTube.getArray())
        if len(funcForm) != nPeaks:
            raise 1
        for val in funcForm:
            if val not in [1,2]:
                raise 2
    except:
        raise RuntimeError("Wrong argument FuncForm. It expects a list of values describing the form of everysingle peaks. So, for example, if there are three peaks where the first is a peak and the followers as edge, funcForm = [1, 2, 2]. Currently, it is defined 1-Gaussian Peak, 2 - Edge. The knownPos has %d elements and the given funcForm has %d."%(nPeaks, len(funcForm)))

    #apply the functional form to the ideal Tube
    idealTube.setForm(funcForm)

    # check the FITPAR parameter (optional)
    # if the FITPAR is given, than it will just pass on, if the FITPAR is
    # not given, it will create a FITPAR 'guessing' the centre positions,
    # and allowing the find peaks calibration methods to adjust the parameter
    # for the peaks automatically
    if kwargs.has_key(FITPAR):
        fitPar = kwargs[FITPAR]
        #fitPar must be a TubeCalibFitParams
        if not isinstance(fitPar, TubeCalibFitParams):
            raise RuntimeError("Wrong argument %s. This argument, when given, must be a valid TubeCalibFitParams object"%FITPAR)
    else:
        # create a fit parameters guessing centre positions
        # the guessing obeys the following rule:
        #
        # centre_pixel = known_pos * ndets/tube_length + ndets / 2
        #
        # Get tube length and number of detectors
        tube_length = tubeSet.getTubeLength(0)
        #ndets = len(wsp_index_for_tube0)
        id1, ndets, step = tubeSet.getDetectorInfoFromTube(0)

        known_pos = idealTube.getArray()
        # position of the peaks in pixels
        centre_pixel = known_pos * ndets/tube_length + ndets * 0.5

        fitPar = TubeCalibFitParams(centre_pixel)
        # make it automatic, it means, that for every tube,
        # the parameters for fit will be re-evaluated, from the first
        # guess positions given by centre_pixel
        fitPar.setAutomatic(True)


    # check the MARGIN paramter (optional)
    if kwargs.has_key(MARGIN):
        try:
            margin = float(kwargs[MARGIN])
        except:
            raise RuntimeError("Wrong argument %s. It was expected a number!"%MARGIN)
        fitPar.setMargin(margin)

    #deal with RANGELIST parameter
    if kwargs.has_key(RANGELIST):
        rangeList = kwargs[RANGELIST]
        if isinstance(rangeList,int):
            rangeList = [rangeList]
        try:
            # this deals with list and tuples and iterables to make sure
            # rangeList becomes a list
            rangeList = list(rangeList)
        except:
            raise RuntimeError("Wrong argument %s. It expects a list of indexes for calibration"%RANGELIST)
    else:
        rangeList = range(tubeSet.getNumTubes())

    # check if the user passed the option calibTable
    if kwargs.has_key(CALIBTABLE):
        calibTable = kwargs[CALIBTABLE]
        #ensure the correct type is passed
        # if a string was passed, transform it in mantid object
        if isinstance(calibTable,str):
            calibTable = mtd[calibTable]
        #check that calibTable has the expected form
        try:
            if not isinstance(calibTable,ITableWorkspace):
                raise 1
            if calibTable.columnCount() != 2:
                raise 2
            colNames = calibTable.getColumnNames()
            if colNames[0] != 'Detector ID' or colNames[1] != 'Detector Position':
                raise 3
        except:
            raise RuntimeError("Invalid type for %s. The expected type was ITableWorkspace with 2 columns(Detector ID and Detector Positions)" % CALIBTABLE)
    else:
        calibTable = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
        # "Detector ID" column required by ApplyCalibration
        calibTable.addColumn(type="int",name="Detector ID")
        # "Detector Position" column required by ApplyCalibration
        calibTable.addColumn(type="V3D",name="Detector Position")


    #deal with plotTube option
    if kwargs.has_key(PLOTTUBE):
        plotTube = kwargs[PLOTTUBE]
        if isinstance(plotTube, int):
            plotTube = [plotTube]
        try:
            plotTube = list(plotTube)
        except:
            raise RuntimeError("Wrong argument %s. It expects an index (int) or a list of indexes" %PLOTTUBE)
    else:
        plotTube = []

    #deal with minimun tubes sizes
    if kwargs.has_key(EXCLUDESHORT):
        excludeShortTubes = kwargs[EXCLUDESHORT]
        try:
            excludeShortTubes = float(excludeShortTubes)
        except:
            raise RuntimeError("Wrong argument %s. It expects a float value for the minimun size of tubes to be calibrated")
    else:
        #a tube with length 0 can not be calibrated, this is the minimun value
        excludeShortTubes = 0.0

    #deal with OVERRIDEPEAKS parameters
    if kwargs.has_key(OVERRIDEPEAKS):
        overridePeaks = kwargs[OVERRIDEPEAKS]
        try:
            nPeaks = len(idealTube.getArray())
            # check the format of override peaks
            if not isinstance(overridePeaks, dict):
                raise 1
            for key in overridePeaks.keys():
                if not isinstance(key,int):
                    raise 2
                if key < 0 or key >= tubeSet.getNumTubes():
                    raise 3
                if len(overridePeaks[key]) != nPeaks:
                    raise 4
        except:
            raise RuntimeError("Wrong argument %s. It expects a dictionary with key as the tube index and the value as a list of peaks positions. Ex (3 peaks): overridePeaks = {1:[2,5.4,500]}"%OVERRIDEPEAKS)
    else:
        overridePeaks = dict()


    # deal with FITPOLIN parameter
    if kwargs.has_key(FITPOLIN):
        polinFit = kwargs[FITPOLIN]
        if polinFit not in [1, 2,3]:
            raise RuntimeError("Wrong argument %s. It expects a number 1 for linear, 2 for quadratic, or 3 for 3rd polinomial order when fitting the pixels positions agains the known positions" % FITPOLIN)
    else:
        polinFit = 2

    # deal with OUTPUT PEAK
    deletePeakTableAfter = False
    if kwargs.has_key(OUTPUTPEAK):
        outputPeak = kwargs[OUTPUTPEAK]
    else:
        outputPeak = False
    if isinstance(outputPeak, ITableWorkspace):
        if outputPeak.columnCount() < len(idealTube.getArray()):
            raise RuntimeError("Wrong argument %s. It expects a boolean flag, or a ITableWorksapce with columns (TubeId, Peak1,...,PeakM) for M = number of peaks given in knownPositions" % OUTPUTPEAK)
    else:
        if not outputPeak:
            deletePeakTableAfter = True
        # create the output peak table
        outputPeak = CreateEmptyTableWorkspace(OutputWorkspace="PeakTable")
        outputPeak.addColumn(type='str',name='TubeId')
        for i in range(len(idealTube.getArray())):
            outputPeak.addColumn(type='float',name='Peak%d'%(i+1))

    getCalibration(ws, tubeSet, calibTable, fitPar, idealTube, outputPeak,
        overridePeaks, excludeShortTubes, plotTube, rangeList, polinFit)

    if deletePeakTableAfter:
        DeleteWorkspace(str(outputPeak))
        return calibTable
    else:
        return calibTable, outputPeak


def savePeak(peakTable, filePath):
    """
    Allows to save the peakTable to a text file.

    :param peakTable: peak table as the workspace table provided by calibrated method, as in the example:

    .. code-block:: python

       calibTable, peakTable = calibrate(..., outputPeak=peakTable)
       savePeak(peakTable, 'myfolder/myfile.txt')

    :param filePath: where to save the file. If the filePath is not given as an absolute path, it will be considered relative to the defaultsave.directory.

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
    if isinstance(peakTable,str):
        peakTable = mtd[peakTable]
    nPeaks = peakTable.columnCount()-1
    peaksNames = ['Peak%d'%(i+1) for i in range(nPeaks)]

    for line in range(peakTable.rowCount()):
        row = peakTable.row(line)
        peak_values = [row[k] for k in peaksNames]
        tube_name = row['TubeId'].replace(' ','%20')
        print >> pFile, tube_name, peak_values

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
    #split the entries to the main values:
    # For example:
    # MERLIN/door1/tube_1_1 [34.199347724575574, 525.5864438725401, 1001.7456248836971]
    # Will be splited as:
    # ['MERLIN/door1/tube_1_1', '', '34.199347724575574', '', '525.5864438725401', '', '1001.7456248836971', '', '', '']
    pattern = re.compile('[\[\],\s\r]')
    saveDirectory = config['defaultsave.directory']
    pfile = os.path.join(saveDirectory, file_name)
    for line in open(pfile,'r'):
        #check if the entry is a comment line
        if line.startswith('#'):
            continue
        #split all values
        line_vals = re.split(pattern,line)
        id_ = str(line_vals[0]).replace('%20',' ')
        if id_ == '':
            continue
        try:
            f_values = [float(v) for v in line_vals[1:] if v!='']
        except ValueError:
            #print 'Wrong format: we expected only numbers, but receive this line ',str(line_vals[1:])
            continue

        loaded_file.append((id_,f_values))
    return loaded_file

