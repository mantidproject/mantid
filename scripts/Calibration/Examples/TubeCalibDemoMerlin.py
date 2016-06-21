#pylint: disable=invalid-name
"""
Tube Calibration Demonstration program for MERLIN.

.. attention::

  MERLIN instruments are loaded with already calibrated values. The calibration works nicelly with these files,
  but if you want to see the uncalibrated file you can do it.
  Look at `How to reset detectors calibration <http://www.mantidproject.org/How_to_reset_detectors_calibration>`_.

In this example, the calibration of the whole MERLIN instrument is shown. It demonstrate how to
use :py:func:`tube.calibrate` to calibrate MERLIN tubes.

Opening the calibrated data in the Instrument View, it is possible to group the tubes in some
common regions:

* Doors 9 and 8 are similar and can be calibrated together using 7 key points
* Doors 7,6,5,4, 2 and 1 can be calibrated using 9 key points.
* Door 3 is particular, because it is formed with some smaller tubes as well as some large tubes.


This example shows:

 * How to calibrate regions of the instrument separetelly.
 * How to use **calibTable** parameter to append information in order to create a calibration table for the whole instrument.
 * How to use the **outputPeak** to check how the calibration is working as well as the usage of analisePeakTable method to look into the details of the operation to improve the calibration.
 * It deals with defining different known positions for the different tube lengths.


The output of this examples shows an improvement in relation to the previous calibrated instrument.


.. image:: /images/calibratedMantidMerlin.png

The previous calibrated instrument view:

.. image:: /images/calibratedCurrentMerlin.png


.. sectionauthor::  Karl Palmen - ISIS

.. sectionauthor:: Gesner Passos - ISIS
"""
import numpy
from mantid.simpleapi import *
import tube


def analisePeakTable(pTable, peaksName='Peaks'):
    print 'parsing the peak table'
    n = len(pTable)
    peaks = pTable.columnCount() -1
    peaksId = n*['']
    data = numpy.zeros((n,peaks))
    line = 0
    for row in pTable:
        data_row = [row['Peak%d'%(i)] for i in range(1,peaks+1)]
        data[line,:] = data_row
        peaksId[line] = row['TubeId']
        line+=1
    # data now has all the peaks positions for each tube
    # the mean value is the expected value for the peak position for each tube
    expected_peak_pos = numpy.mean(data,axis=0)
    print expected_peak_pos
    #calculate how far from the expected position each peak position is
    distance_from_expected =  numpy.abs(data - expected_peak_pos)

    Peaks = CreateWorkspace(range(n),distance_from_expected,NSpec=peaks, OutputWorkspace=peaksName)
    check = numpy.where(distance_from_expected > 10)[0]
    problematic_tubes = list(set(check))
    print 'Tubes whose distance is far from the expected value: ', problematic_tubes


def calibrateMerlin(filename):
  # == Set parameters for calibration ==

    rangeLower = 3000 # Integrate counts in each spectra from rangeLower to rangeUpper
    rangeUpper = 20000 #

  # Get calibration raw file and integrate it
    rawCalibInstWS = LoadRaw(filename)    #'raw' in 'rawCalibInstWS' means unintegrated.
    print "Integrating Workspace"
    CalibInstWS = Integration( rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper )
    DeleteWorkspace(rawCalibInstWS)
    print "Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate"

  # the known positions are given in pixels inside the tubes and transformed to provide the positions
  # with the center of the tube as the origin
    knownPositions = 2.92713867188*(numpy.array([ 27.30074322, 92.5,    294.65178585,    362.37861919 , 512.77103043    ,663.41425323, 798.3223896,     930.9, 997.08480835])/1024 - 0.5)
    funcForm = numpy.array([2,2,1,1,1,1,1,2,2],numpy.int8)
  # The calibration will follow different steps for sets of tubes

  # For the door9, the best points to define the known positions are the 1st edge, 5 peaks, last edge.
    points7 = knownPositions[[0,2,3,4,5,6,8]]
    points7func = funcForm[[0,2,3,4,5,6,8]]

    door9pos = points7
    door9func = points7func
    CalibratedComponent = 'MERLIN/door9'    # door9
  # == Get the calibration and put results into calibration table ==
  # also put peaks into PeakFile
    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, door9pos, door9func,
        outputPeak=True,
        margin=30,
        rangeList=range(20) # because 20, 21, 22, 23 are defective detectors
        )
    print "Got calibration (new positions of detectors) and put slit peaks into file TubeDemoMerlin01.txt"
    analisePeakTable(peakTable, 'door9_tube1_peaks')

  # For the door8, the best points to define the known positions are the 1st edge, 5 peaks, last_edge
    door8pos = points7
    door8func = points7func
    CalibratedComponent = 'MERLIN/door8'
    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, door8pos,
        door8func,
    outputPeak = True, #change to peakTable to append to peakTable
    calibTable = calibrationTable,
    margin = 30)
    analisePeakTable(peakTable, 'door8_peaks')

  # For the doors 7,6,5,4, 2, 1 we may use the 9 points
    doorpos = knownPositions
    doorfunc = funcForm
    CalibratedComponent = ['MERLIN/door%d'%(i) for i in [7,6,5,4, 2, 1]]
    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, doorpos,\
        doorfunc,\
    outputPeak = True,\
    calibTable = calibrationTable,\
    margin = 30)
    analisePeakTable(peakTable, 'door1to7_peaks')

  # The door 3 is a special case, because it is composed by diffent kind of tubes.
  # door 3 tubes: 5_8, 5_7, 5_6, 5_5, 5_4, 5_3, 5_2, 5_1, 4_8, 4_7, 4_6, 4_5, 4_4, 4_3, 4_2, 4_1, 3_8, 3_7, 3_6, 3_5, 3_4
  # obeys the same rules as the doors 7, 6, 5, 4, 2, 1
  # For the tubes 3_3, 3_2, 3_1 -> it is better to skip the central peak
  # For the tubes 1_x (smaller tube below), it is better to take the final part of known positions: peak4,peak5,edge6,edge7
  # For the tubes 2_x (smaller tube above, it is better to take the first part of known positions: edge1, edge2, peak1,peak2

  # NOTE: the smaller tubes they have length = 1.22879882813, but 1024 detectors
  # so we have to correct the known positiosn by multiplying by its lenght and dividing by the longer dimension

    from tube_calib_fit_params import TubeCalibFitParams

  # calibrating tubes 1_x
    CalibratedComponent = ['MERLIN/door3/tube_1_%d'%(i) for i in range(1,9)]

    half_diff_center = (2.92713867188 -1.22879882813)/2    # difference among the expected center position for both tubes

  # here a little bit of attempts is necessary. The efective center position and lengh is different for the calibrated tube, that
  # is the reason, the calibrated values of the smaller tube does not seems aligned with the others. By, finding the 'best' half_diff_center
  # value, the alignment occurs nicely.
    half_diff_center = 0.835 #

  # the knownpositions were given with the center of the bigger tube as origin, to convert
  # to the center of the upper tube as origin is necessary to subtract them with  the half_diff_center
    doorpos = knownPositions[[5,6,7,8]] - half_diff_center
    doorfunc = [1,1,2,2]
  # for the smal tubes, automatically searching for the peak position in pixel was not working quite well,
  # so we will give the aproximate position for these tubes through fitPar argument
    fitPar = TubeCalibFitParams([216, 527, 826, 989])
    fitPar.setAutomatic(True)

    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, doorpos,\
    doorfunc,\
    outputPeak = True,\
    fitPar = fitPar,\
    calibTable = calibrationTable,\
    margin = 30)
    analisePeakTable(peakTable, 'door3_tube1_peaks')

  # calibrating tubes 2_x
    CalibratedComponent = ['MERLIN/door3/tube_2_%d'%(i) for i in range(1,9)]
  # the knownpositions were given with the center of the bigger tube as origin, to convert
  # to the center of the lower tube as origin is necessary to sum them with  (len_big - len_small)/2
    doorpos = knownPositions[[0,1,2,3]] + half_diff_center
  # print doorpos
    doorfunc = [2,2,1,1]

  # for the smal tubes, automatically searching for the peak position in pixel was not working quite well,
  # so we will give the aproximate position for these tubes through fitPar argument
    fitPar = TubeCalibFitParams([50, 202, 664, 815])
    fitPar.setAutomatic(True)

    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, doorpos,\
    doorfunc,\
    outputPeak = True,\
    calibTable = calibrationTable,\
    fitPar = fitPar,\
    margin = 30)

    analisePeakTable(peakTable, 'door3_tube2_peaks')


  # calibrating tubes 3_3,3_2,3_1
    CalibratedComponent = ['MERLIN/door3/tube_3_%d'%(i) for i in [1,2,3]]
    doorpos = knownPositions[[0,1,2,3,5,6,7,8]]
    doorfunc = funcForm[[0,1,2,3,5,6,7,8]]
    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, doorpos,\
    doorfunc,\
    outputPeak = True,\
    calibTable = calibrationTable,\
    margin = 30)
    analisePeakTable(peakTable, 'door3_123_peaks')

  # calibrating others inside door3
  # 5_8, 5_7, 5_6, 5_5, 5_4, 5_3, 5_2, 5_1, 4_8, 4_7, 4_6, 4_5, 4_4, 4_3, 4_2, 4_1, 3_8, 3_7, 3_6, 3_5, 3_4
    part_3 = ['MERLIN/door3/tube_3_%d'%(i) for i in [4,5,6,7,8]]
    part_4 = ['MERLIN/door3/tube_4_%d'%(i) for i in range(1,9)]
    part_5 = ['MERLIN/door3/tube_5_%d'%(i) for i in range(1,9)]
    CalibratedComponent = part_3 + part_4 + part_5
    doorpos = knownPositions
    doorfunc = funcForm
    calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponent, doorpos,\
        doorfunc,\
    outputPeak = True,\
    calibTable = calibrationTable,\
    margin = 30)
    analisePeakTable(peakTable, 'door3_peaks')

  # == Apply the Calibation ==
    ApplyCalibration( Workspace=CalibInstWS, PositionTable=calibrationTable)
    print "Applied calibration"

  # == Save workspace ==
  #SaveNexusProcessed( CalibInstWS, 'TubeCalibDemoMerlinResult.nxs',"Result of Running TCDemoMerlin.py")
  #print "saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMerlinResult.nxs"


if __name__=="__main__":
    filename = 'MER12024.raw' # Calibration run ( found in \\isis\inst$\NDXMERLIN\Instrument\data\cycle_11_5 )
    calibrateMerlin(filename)
