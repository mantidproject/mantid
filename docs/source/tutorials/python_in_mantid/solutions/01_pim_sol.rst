.. _01_pim_sol:

============================
Python in Mantid: Exercise 1
============================

Using ISIS Data
===============

Removing the HRPD Prompt Pulse
------------------------------

The HRPD data contains a spike every 20 milliseconds. While this is nicely localized in time-of-flight, it is not the case upon conversion to d-spacing.

The aim of this exercise is to use Mantid to write a script, not point-and-click, to exclude the contribution from the pulse.

.. code-block:: python

    # The input data set
    inputData = "HRP39182"
    # Load the file
    Load(Filename=inputData+".RAW",OutputWorkspace=inputData,Cache="If Slow")

    # First do the analysis without prompt pulse removal so that we can compare the difference
    # Align the detectors (incoporates unit conversion to d-Spacing)
    cal_file = "hrpd_new_072_01_corr.cal"
    AlignDetectors(InputWorkspace=inputData,OutputWorkspace="aligned-withpulse",CalibrationFile=cal_file)
    # Focus the data
    DiffractionFocussing(InputWorkspace="aligned-withpulse",OutputWorkspace="focussed-withpulse",GroupingFileName=cal_file)

    # Plot a spectrum. As each pulse is removed below, the graph will update
    plotSpectrum(inputData,0)

    # Remove the prompt pulse, which occurs at at 20,000 microsecond intervals. The bin width comes from a quick look at the data
    for i in range(0,5):
      min = 19990 + (i*20000)
      max = 20040 + (i*20000)
      MaskBins(InputWorkspace=inputData,OutputWorkspace=inputData,XMin=min,XMax=max)

    # Align the detectors (on the data with the pulse removed incoporates unit conversion to d-Spacing)
    AlignDetectors(InputWorkspace=inputData,OutputWorkspace="aligned-withoutpulse",CalibrationFile=cal_file)
    # Focus the data
    DiffractionFocussing(InputWorkspace="aligned-withoutpulse",OutputWorkspace="focussed-withoutpulse",GroupingFileName=cal_file)

    # Now plot a focussed spectrum with and without prompt peak removal so that you can see the difference
    plotSpectrum(["focussed-withoutpulse","focussed-withpulse"],0)


Using SNS Data
==============

.. code-block:: python

    run = Load('PG3_4871_event')
    nevents = run.getNumberEvents()
    logger.information(str(nevents))
    filtered = FilterBadPulses(run, LowerCutoff=99.5)
    aligned = AlignDetectors(InputWorkspace=filtered, CalibrationFile='PG3_golden.cal')
    rebinned = Rebin(InputWorkspace=aligned, Params=[1.4,-0.0004, 8])
    focused = DiffractionFocussing(InputWorkspace=rebinned, GroupingFileName='PG3_golden.cal')
    compressed = CompressEvents(InputWorkspace=focused)
    nevents = compressed.getNumberEvents()
    logger.notice(str(nevents))