.. _07_pim_ex_1:

============================
Python in Mantid: Exercise 1
============================

The aim of this exercise is to show some simple examples of data exploration

.. contents:: Table of contents
    :local:

A - ISIS Data
=============

The HRPD data contains a spike (pulse) every 20 milliseconds. While this is nicely localized in time-of-flight, it is not the case upon conversion to d-spacing.

The aim of this exercise is to use Mantid to write a script, which excludes this contribution from the pulse.

#. :ref:`algm-Load` - Load the given HRPD data set, HRP39182.RAW into a workspace called 'HRP39182'
#. :ref:`algm-MaskBins` - Mask out the bins corresponding to the pulse with XMin=19990 and XMax=20040
#. Repeat the previous step for the other 4 pulses, each of which is 20000 microseconds after the previous. *All MaskBins executions should happen on the same InputWorkspace so that all 5 pulses are masked from the same workspace. Hint: A loop might be useful.*
#. :ref:`algm-ApplyDiffCal`- Correct the masked workspace for small variations in detector position, using the calibration file `hrpd_new_072_01_corr.cal`.
#. :ref:`algm-ConvertUnits` - Convert unit to dSpacing
#. :ref:`algm-DiffractionFocussing` - Focus the data in the masked workspace using the same cal file as the previous step (called a grouping file here)

Bonus: Can you run the Align-Focus process on the original, unmasked data and :ref:`algm-Minus` the final workspace processed without the pulse from the final workspace processed with the pulse? Plot the difference workspace with the final processed data to compare.

B - SNS Data
============

Filter, process and compress event data.

#. :ref:`algm-LoadEventNexus` - Load the given POWGEN data set, PG3_4871. If you need to reduce the number of events loaded, select only the first 4000 seconds of the run.
#. Log the number of events in the Messages Box with the command `logger.notice("message")` (The function to get the number of events from a workspace called `ws` is `ws.getNumberEvents()`.
#. :ref:`algm-FilterBadPulses` - Remove events that occurred while the accelerator was resetting, by setting the LowerCutoff to 99.5 % of the average beam proton charge.
#. :ref:`algm-ApplyDiffCal`- Correct the masked workspace for small variations in detector position, using the calibration file `PG3_golden.cal`.
#. :ref:`algm-ConvertUnits` - Convert unit to dSpacing.
#. :ref:`algm-Rebin` - Bin the data in d-spacing from 1.4 to 8 angstroms using logarithmic binning of .0004.
#. :ref:`algm-DiffractionFocussing` - Focus the data in the workspace using the same cal file as the previous step (PG3_golden.cal).
#. :ref:`algm-CompressEvents` - Saves some memory. Again, extract and log the number of events.


C - ILL Data
============

Merge, mask, correct and compare.

#. :ref:`algm-Load` - Load the given IN6 data sets, '164198.nxs', '164199.nxs' and '164200.nxs' into workspaces named after the filename.
#. :ref:`algm-MergeRuns` - Merge all the previously loaded data sets into a single workspaced called 'data_merged'.
#. :ref:`algm-MaskDetectors` - Remove bad spectra indices : 1,2,3,4,5,6,11,14,30,69,90,93,95,97,175,184,190,215,216,217,251,252,253,255,289,317,335 and 337.
#. :ref:`algm-MultiplyRange` - Calculate sample transmission of 95%.
#. :ref:`algm-ConvertUnits` - Convert the data from TOF to Delta Energy. (Find the Efixed value as Ei in the Sample Logs for the workspace)
#. :ref:`algm-DetectorEfficiencyCorUser` - Correct the data with the detector efficiency for this instrument.
#. Compare the corrected with the unccorrected data, say bin 4 of spectrum number 7 (workspace index 6).

:ref:`Solutions <01_pim_sol>`
