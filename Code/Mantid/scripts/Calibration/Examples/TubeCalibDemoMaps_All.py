#pylint: disable=invalid-name
"""
Tube Calibration Demonstration for MAPS instrument.

This module group many examples and also demonstrate how to work with
the :func:`tube.calibrate`.

It starts from the simplest way to perform a calibration. It them increase the complexity
of dealing with real calibration work, when particularities of the instrument must be considered

At the end, it gives a suggestion on a techinique to investigate and improve the calibration itself.

Minimal Input
~~~~~~~~~~~~~

.. autofunction:: minimalInput

Provide Expected Value
~~~~~~~~~~~~~~~~~~~~~~

.. autofunction:: provideTheExpectedValue

Change Margin and Expected Value
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. autofunction:: changeMarginAndExpectedValue

Improving Calibration Single Tube
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. autofunction:: improvingCalibrationSingleTube

Improving Calibration of List of Tubes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. autofunction:: improvingCalibrationOfListOfTubes

Calibrating Window B2
~~~~~~~~~~~~~~~~~~~~~
.. autofunction:: calibrateB2Window

Complete Calibration
~~~~~~~~~~~~~~~~~~~~
.. autofunction:: completeCalibration

Calibration technique: Finding tubes not well calibrated
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. autofunction:: findThoseTubesThatNeedSpecialCareForCalibration(filename)


.. sectionauthor::  Karl Palmen - ISIS

.. sectionauthor:: Gesner Passos - ISIS

"""

from mantid.simpleapi import *
import tube
import numpy

def loadingStep(filename):
    filename = str(filename)
    rangeLower = 2000 # Integrate counts in each spectra from rangeLower to rangeUpper
    rangeUpper = 10000 #
    # Get calibration raw file and integrate it
    rawCalibInstWS = Load(filename)  #'raw' in 'rawCalibInstWS' means unintegrated.
    print "Integrating Workspace"
    CalibInstWS = Integration( rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper )
    DeleteWorkspace(rawCalibInstWS)
    print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate"
    return CalibInstWS


def minimalInput(filename):
    """
    Simplest way of calling :func:`tube.calibrate`


    The minimal input for the calibration is the integrated workspace
    and the knwon positions.

    Eventhough it is easy to call, the calibration performs well, but there are ways to improve
    the results, as it is explored after.

    .. image:: /images/outputOfMinimalInput.png

    """
    CalibInstWS = loadingStep(filename)
    # == Set parameters for calibration ==
    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MAPS'  # Calibrate all
    # define the known positions and function factor (edge, peak, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16,-0.00, 0.16, 0.50 ],[2,1,1,1,2]

    # == Get the calibration and put results into calibration table ==
    calibrationTable = tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor)
    # == Apply the Calibation ==
    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)

def provideTheExpectedValue(filename):
    """
    Giving the expected value for the position of the peaks in pixel.

    The :func:`~Examples.minimalInput` let to the calibrate to guess the position of the pixels
    among the tubes. Altough it works nicelly, providing these expected values may improve the results.
    This is done through the **fitPar** parameter.
    """
    from tube_calib_fit_params import TubeCalibFitParams
    CalibInstWS = loadingStep(filename)
    # == Set parameters for calibration ==
    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MAPS'  # Calibrate all
    # define the known positions and function factor (edge, peak, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16,-0.00, 0.16, 0.50 ],[2,1,1,1,2]

    # the expected positions in pixels for the special points
    expectedPositions = [4.0, 85.0, 128.0, 161.0, 252.0]
    fitPar = TubeCalibFitParams(expectedPositions)
    fitPar.setAutomatic(True)

    # == Get the calibration and put results into calibration table ==
    calibrationTable = tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,
                                      fitPar=fitPar)
    # == Apply the Calibation ==
    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)

def changeMarginAndExpectedValue(filename):
    """
    To fit correcly, it is important to have a good window around the peak. This windown is defined
    by the **margin** parameter.

    This examples shows how the results worsen if we change the margin from its default value **15**
    to **10**.

    It shows how to see the fitted values using the **plotTube** parameter.

    It will also output the peaks position and save them, through the **outputPeak** option and
    the :func:`tube.savePeak` method.

    An example of the fitted data compared to the acquired data to find the peaks positions:

    .. image:: /images/calibratePlotFittedData.png

    The result deteriorate, as you can see:

    .. image:: /images/calibrateChangeMarginAndExpectedValue.png

    """
    from tube_calib_fit_params import TubeCalibFitParams
    CalibInstWS = loadingStep(filename)
    # == Set parameters for calibration ==
    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MAPS'  # Calibrate all
    # define the known positions and function factor (edge, peak, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16,-0.00, 0.16, 0.50 ],[2,1,1,1,2]

    # the expected positions in pixels for the special points
    expectedPositions = [4.0, 85.0, 128.0, 161.0, 252.0]
    fitPar = TubeCalibFitParams(expectedPositions)
    fitPar.setAutomatic(True)

    # == Get the calibration and put results into calibration table ==
    calibrationTable, peakTable= tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,
                                                fitPar=fitPar, plotTube=[1,10,100], outputPeak=True, margin=10)
    # == Apply the Calibation ==
    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)

    tube.savePeak(peakTable, 'TubeDemoMaps01.txt')


def improvingCalibrationSingleTube(filename):
    """
    The :func:`~Examples.provideTheExpectedValue` provided a good solution, but there are few
    tubes whose calibration was not so good.

    This method explores how to deal with these tubes.

    First of all, it is important to identify the tubes that did not work well.

    From the outputs of provideTheExpectedValue, looking inside the instrument tree,
    it is possible to list all the tubes that are not so good.

    Unfortunatelly, they do not have a single name identifier.
    So, locating them it is a little bit trickier.
    The :func:`~Examples.findThoseTubesThatNeedSpecialCareForCalibration` shows one way of finding those
    tubes.     The index is the position inside the PeakTable.

    For this example, we have used inspection from the Instrument View.
    One of them is inside the A1_Window, 3rd PSD_TUBE_STRIP 8 pack up, 4th PSD_TUBE_STRIP: Index = 8+8+4 - 1 = 19.

    In this example, we will ask the calibration to run the calibration only for 3 tubes
    (indexes 18,19,20). Them, we will check why the 19 is not working well. Finally, we will try to
    provide another peaks position for this tube,
    and run the calibration again for these tubes, to improve the results.

    This example shows how to use **overridePeaks** option
    """
    from tube_calib_fit_params import TubeCalibFitParams
    import time
    CalibInstWS = loadingStep(filename)
    # == Set parameters for calibration ==
    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MAPS'  # Calibrate all
    # define the known positions and function factor (edge, peak, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16,-0.00, 0.16, 0.50 ],[2,1,1,1,2]

    # the expected positions in pixels for the special points
    expectedPositions = [4.0, 85.0, 128.0, 161.0, 252.0]
    fitPar = TubeCalibFitParams(expectedPositions)
    fitPar.setAutomatic(True)

    # == Get the calibration and put results into calibration table ==
    calibrationTable, peakTable= tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,
                                                fitPar=fitPar, outputPeak=True, plotTube=[18,19,20], rangeList=[18,19,20])

    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)


    # reload to reset the calibration applied
    CalibInstWS = loadingStep(filename)
    # looking into the second line of calibrationTable, you will see that it defines the peaks for the first position
    # as 14.9788, -0.303511, 9.74828
    # let's change the peak from  -0.303511 to 8.14

    #to override the peaks definition, we use the overridePeaks
    overridePeaks = {19: [8.14, 80.9771, 123.221, 164.993, 245.717]}

    # == Get the calibration and put results into calibration table ==
    # we will not plot anymore, because it will not plot the overrided peaks
    calibrationTable, peakTable= tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,
                                                fitPar=fitPar, outputPeak=True, rangeList=[18,19,20], overridePeaks=overridePeaks)

    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
    #check using the InstrumentView and you will see that it is better than before


def improvingCalibrationOfListOfTubes(filename):
    """
    Analysing the result of provideTheExpectedValue it was seen that the calibration
    of some tubes was not good.

    .. note::
          This method list some of them, there are a group belonging to window B2 that shows
          only 2 peaks that are not dealt with here.

    If first plot the bad ones using the **plotTube** option. It them, find where they fail, and how
    to correct their peaks, using the **overridePeaks**.
    If finally, applies the calibration again with the points corrected.
    """
    from tube_calib_fit_params import TubeCalibFitParams

    not_good = [19,37, 71, 75, 181, 186, 234, 235, 245, 273, 345]

    CalibInstWS = loadingStep(filename)
    # == Set parameters for calibration ==
    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MAPS'  # Calibrate all
    # define the known positions and function factor (edge, peak, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16,-0.00, 0.16, 0.50 ],[2,1,1,1,2]

    # the expected positions in pixels for the special points
    expectedPositions = [4.0, 85.0, 128.0, 161.0, 252.0]
    fitPar = TubeCalibFitParams(expectedPositions)
    fitPar.setAutomatic(True)

    # == Get the calibration and put results into calibration table ==
    #calibrationTable, peakTable= tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,
    #	fitPar=fitPar, outputPeak=True, plotTube=not_good, rangeList=not_good)

    #CalibInstWS = loadingStep(filename)

    # it is defined as the mean values around the neighbours
    define_peaks = {19:[10, 80.9771, 123.221, 164.993, 245.717], # the first one was bad
        37: [6.36, 80.9347, 122.941, 165.104, 248.32], # the first one was bad
        71: [8.62752, 85.074, 124.919, 164.116, 246.82 ], # the last one was bad - check if we can inprove
        75: [14.4285, 90.087, 128.987, 167.047, 242.62], # the last one was bad - check if we can inprove
        181: [11.726, 94.0496, 137.816,  180, 255], # the third peak was lost
        186:[11.9382, 71.5203, 107, 147.727, 239.041], #lost the second peak
        234: [4.84, 82.7824, 123.125, 163.945, 241.877], # the first one was bad
        235: [4.84, 80.0077, 121.002, 161.098, 238.502], # the first one was bad
        245: [9.88089, 93.0593, 136.911, 179.5, 255], # the third peak was bad
        273: [18.3711, 105.5, 145.5, 181.6, 243.252], # lost first and third peaks
        345: [4.6084, 87.0351, 128.125, 169.923, 245.3] # the last one was bad
        }
    calibrationTable, peakTable= tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,
                                                fitPar=fitPar, outputPeak=True, overridePeaks=define_peaks)

    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)

def calibrateB2Window(filename):
    """
    There are among the B2 window tubes, some tubes that are showing only 2 strips.

    Those tubes must be calibrated separated, as the known positions are not valid.

    This example calibrate them, using only 4 known values: 2 edges and 2 peaks.

    Run this example, and them see the worksapce in the calibrated instrument and you will see
    how it worked.

    The picture shows the output, look that only a section of the B2 Window was calibrated.

    .. image:: /images/calibrateB2Window.png

    """
    from tube_calib_fit_params import TubeCalibFitParams
    # b2 with 2 peaks range
    b2_range = range(196,212) + range(222,233)

    CalibInstWS = loadingStep(filename)
    # == Set parameters for calibration ==
    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MAPS'  # Calibrate all
    # define the known positions and function factor (edge, peak, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16, 0.16, 0.50 ],[2,1,1,2]

    # the expected positions in pixels for the special points
    expectedPositions = [4.0, 85.0, 161.0, 252.0]
    fitPar = TubeCalibFitParams(expectedPositions)
    fitPar.setAutomatic(True)

    # == Get the calibration and put results into calibration table ==
    calibrationTable, peakTable= tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,\
                fitPar=fitPar, outputPeak=True, plotTube=[b2_range[0], b2_range[-1]], rangeList=b2_range)

    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)



def findThoseTubesThatNeedSpecialCareForCalibration(filename):
    """
    The example :func:`provideTheExpectedValue` has shown its capability to calibrate almost
    all tubes, but,    as explored in the :func:`improvingCalibrationOfListOfTubes` and
    :func:`improvingCalibrationSingleTube` there are
    some tubes that could not be calibrated using that method.

    The goal of this method is to show one way to find the tubes that will require special care.

    It will first perform the same calibration seen in :func:`provideTheExpectedValue`,
    them, it will process the **peakTable** output of the calibrate method when enabling the
    parameter **outputPeak**.

    It them creates the Peaks workspace, that is the diffence of the peaks position from the
    expected values of the peaks positions for all the tubes. This allows to spot what are the
    tubes whose fitting are outliers in relation to the others.

    .. image:: /images/plotingPeaksDifference.png

    The final result for this method is to output using **plotTube** the result of the fitting
    to all the 'outliers' tubes.
    """
    from tube_calib_fit_params import TubeCalibFitParams
    CalibInstWS = loadingStep(filename)
    # == Set parameters for calibration ==
    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MAPS'  # Calibrate all
    # define the known positions and function factor (edge, peak, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16,-0.00, 0.16, 0.50 ],[2,1,1,1,2]

    # the expected positions in pixels for the special points
    expectedPositions = [4.0, 85.0, 128.0, 161.0, 252.0]
    fitPar = TubeCalibFitParams(expectedPositions)
    fitPar.setAutomatic(True)

    # == Get the calibration and put results into calibration table ==
    calibrationTable, peakTable= tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,
                                                fitPar=fitPar, outputPeak=True)

    # == now, lets investigate the peaks

    #parsing the peakTable to produce a numpy array with dimension (number_of_tubes x number_of_peaks)
    print 'parsing the peak table'
    n = len(peakTable)
    peaksId = n*['']
    data = numpy.zeros((n,5))
    line = 0
    for row in peakTable:
        data_row = [row['Peak%d'%(i)] for i in [1,2,3,4,5]]
        data[line,:] = data_row
        peaksId[line] = row['TubeId']
        line+=1
    # data now has all the peaks positions for each tube
    # the mean value is the expected value for the peak position for each tube
    expected_peak_pos = numpy.mean(data,axis=0)
    #calculate how far from the expected position each peak position is
    distance_from_expected =  numpy.abs(data - expected_peak_pos)

    print 'Creating the Peaks Workspace that shows the distance from the expected value for all peaks for each tube'
    # Let's see these peaks:
    Peaks = CreateWorkspace(range(n),distance_from_expected,NSpec=5)

    # plot all the 5 peaks for Peaks Workspace. You will see that most of the tubes differ
    # at most 12 pixels from the expected values.

    #so let's investigate those that differ more than 12
    # return an array with the indexes for the first axis which is the tube indentification
    check = numpy.where(distance_from_expected > 12)[0]

    #remove repeated values
    #select only those tubes inside the problematic_tubes
    problematic_tubes = list(set(check))

    print 'Tubes whose distance is far from the expected value: ', problematic_tubes

    print 'Calibrating again only these tubes'
    #let's confir that our suspect works
    CalibInstWS = loadingStep(filename)
    calibrationTable = tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,\
    	fitPar=fitPar, rangeList= problematic_tubes, plotTube=problematic_tubes)
    # plot the FittedTube agains TubePlot for each detector and you will see that there were problems on those tubes.


def completeCalibration(filename):
    """
    This example shows how to use some properties of calibrate method to
    join together the calibration done in :func:`provideTheExpectedValue`,
    and improved in :func:`calibrateB2Window`, and :func:`improvingCalibrationOfListOfTubes`.

    It also improves the result of the calibration because it deals with the E door. The
    aquired data cannot be used to calibrate the E door, and trying to do so, produces a bad
    result. In this example, the tubes inside the E door are excluded to the calibration.
    Using the '''rangeList''' option.
    """

    # first step, load the workspace
    from tube_calib_fit_params import TubeCalibFitParams
    CalibInstWS = loadingStep(filename)


    # == Set parameters for calibration ==
    # Set what we want to calibrate (e.g whole intrument or one door )
    CalibratedComponent = 'MAPS'  # Calibrate all

    # define the known positions and function factor (edge, peak, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16,-0.00, 0.16, 0.50 ],[2,1,1,1,2]

    # the expected positions in pixels for the special points
    expectedPositions = [4.0, 85.0, 128.0, 161.0, 252.0]
    fitPar = TubeCalibFitParams(expectedPositions)
    fitPar.setAutomatic(True)


    #execute the improvingCalibrationOfListOfTubes excluding the range of b2 window
    # correct the definition of the peaks for the folowing indexes
    #define_peaks = {19:[10, 80.9771, 123.221, 164.993, 245.717], # the first one was bad
    #	37: [6.36, 80.9347, 122.941, 165.104, 248.32], # the first one was bad
    #	71: [8.62752, 85.074, 124.919, 164.116, 246.82 ], # the last one was bad - check if we can inprove
    #	75: [14.4285, 90.087, 128.987, 167.047, 242.62], # the last one was bad - check if we can inprove
    #	181: [11.726, 94.0496, 137.816,  180, 255], # the third peak was lost
    #	186:[11.9382, 71.5203, 107, 147.727, 239.041], #lost the second peak
    #	234: [4.84, 82.7824, 123.125, 163.945, 241.877], # the first one was bad
    #	235: [4.84, 80.0077, 121.002, 161.098, 238.502], # the first one was bad
    #	245: [9.88089, 93.0593, 136.911, 179.5, 255], # the third peak was bad
    #	273: [18.3711, 105.5, 145.5, 181.6, 243.252],# lost first and third peaks
    #	345: [4.6084, 87.0351, 128.125, 169.923, 245.3]} # the last one was bad
    define_peaks = {19:[10, 80.9771, 123.221, 164.993, 245.717],\
    	37: [6.36, 80.9347, 122.941, 165.104, 248.32],\
    	71: [8.62752, 85.074, 124.919, 164.116, 246.82 ],\
    	75: [14.4285, 90.087, 128.987, 167.047, 242.62],\
    	181: [11.726, 94.0496, 137.816,  180, 255],\
    	186:[11.9382, 71.5203, 107, 147.727, 239.041],\
    	234: [4.84, 82.7824, 123.125, 163.945, 241.877],\
    	235: [4.84, 80.0077, 121.002, 161.098, 238.502],\
    	245: [9.88089, 93.0593, 136.911, 179.5, 255],\
    	273: [18.3711, 105.5, 145.5, 181.6, 243.252],\
    	345: [4.6084, 87.0351, 128.125, 169.923, 245.3]}

    b2_window = range(196,212) + range(222,233)

    complete_range = range(648)

    # this data can not be used to calibrate the E1 window, so, let's remove it.
    e1_window = range(560,577)
    aux = numpy.setdiff1d(complete_range, b2_window)
    # the group that have 3 stripts are all the tubes except the b2 window and e window.
    range_3_strips = numpy.setdiff1d(aux, e1_window)

    calibrationTable, peak3Table= tube.calibrate(CalibInstWS, CalibratedComponent, knownPos, funcFactor,\
    	fitPar=fitPar, outputPeak=True, overridePeaks=define_peaks, rangeList=range_3_strips)

    # now calibrate the b2_window REMOVE SECOND PEAK
    # define the known positions and function factor (edge, peak, peak, edge)
    knownPos, funcFactor = [-0.50,-0.16, 0.16, 0.50 ],[2,1,1,2]

    # the expected positions in pixels for the special points
    expectedPositions = [4.0, 85.0, 161.0, 252.0]
    fitPar = TubeCalibFitParams(expectedPositions)
    fitPar.setAutomatic(True)

    # apply the calibration for the b2_window 2 strips values
    calibrationTable, peak2Table = tube.calibrate(CalibInstWS, CalibratedComponent,
        knownPos,  #these parameters now have only 4 points
        funcFactor,
        fitPar=fitPar,
        outputPeak=True,
        calibTable = calibrationTable, # it will append to the calibTable
        rangeList = b2_window)

    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)

    # == Save workspace ==
    #SaveNexusProcessed( CalibInstWS, path+'TubeCalibDemoMapsResult.nxs',"Result of Running TCDemoMaps.py")
    #print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMapsResult.nxs"





####
## Uncomment one of the following lines to execute one of the examples
#####
filename = 'MAP14919.raw' #found at \\isis\inst$\NDXMAPS\Instrument\data\cycle_09_5
if __name__ == "__main__":
    filename = 'MAP14919.raw'
    #minimalInput(filename)
    #provideTheExpectedValue(filename)
    #changeMarginAndExpectedValue(filename)
    #improvingCalibrationSingleTube(filename)
    #improvingCalibrationOfListOfTubes(filename)
    #calibrateB2Window(filename)
    completeCalibration(filename)
    #findThoseTubesThatNeedSpecialCareForCalibration(filename)

