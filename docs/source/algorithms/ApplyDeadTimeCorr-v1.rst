.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Assuming that the *InputWorkspace* contains measured
counts as a function of time, the algorithm returns a workspace
containing true counts as a function of the same time binning
according to

.. math:: N = \frac{M}{(1-M*(\frac{t_{\mathrm{dead}}}{t_{\mathrm{bin}}*F}))}

where

| :math:`N` = true count
| :math:`M` = measured count
| :math:`t_{\mathrm{dead}}` = dead-time
| :math:`t_{\mathrm{bin}}` = time bin width
| :math:`F` = number of good frames

*DeadTimeTable* is expected to have 2 columns:

1. Integer type, containing spectrum number (not index)
2. Double type, containing :math:`t_{\mathrm{dead}}` value of the spectrum

It is assumed that all bins in the *InputWorkspace* are the same
size (to within reasonable rounding error).
If they are not, the algorithm will exit with an error.

The *InputWorkspace* must contain a sample log ``goodfrm`` (number of good frames) for the algorithm to run successfully.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Applying the correction using custom dead times:**

.. testcode:: ExCustomDeadTimes

   # Load single period of a MUSR run
   input = Load('MUSR0015189.nxs', EntryNumber=1)

   # Remove uninteresting bins
   input = CropWorkspace('input', XMin=0.55, XMax=12)

   # Create a table with some arbitrary dead times
   dead_times = CreateEmptyTableWorkspace()
   dead_times.addColumn('int', 'Spectrum no.')
   dead_times.addColumn('double', 'Deadtime')

   for i in range(1,65):
     dead_times.addRow([i, 0.1])

   output = ApplyDeadTimeCorr('input','dead_times')

   original = Integration('input')
   corrected = Integration('output')

   format_str = 'Spectrum: {0:d}; original: {1:.3f}; corrected: {2:.3f}'

   for s in [0,32,63]:
      print(format_str.format(s, original.readY(s)[0], corrected.readY(s)[0]))

Output:

.. testoutput:: ExCustomDeadTimes

   Spectrum: 0; original: 6643.000; corrected: 7100.833
   Spectrum: 32; original: 10384.000; corrected: 11559.134
   Spectrum: 63; original: 8875.000; corrected: 9724.937

**Example - Applying the correction using dead times stored in the Nexus file:**

.. testcode:: ExLoadedDeadTimes

   # Load a MUSR run
   input = Load('MUSR0015189.nxs', DeadTimeTable='dead_times')

   # Remove uninteresting bins
   input = CropWorkspace('input', XMin=0.55, XMax=12)

   # Apply the loaded dead times
   output = ApplyDeadTimeCorr('input','dead_times')

   original = Integration(input.getItem(0))
   corrected = Integration(output.getItem(0))

   format_str = 'Spectrum: {0:d}; original: {1:.3f}; corrected: {2:.3f}'

   for s in [0,32,63]:
      print(format_str.format(s, original.readY(s)[0], corrected.readY(s)[0]))

Output:

.. testoutput:: ExLoadedDeadTimes

   Spectrum: 0; original: 6643.000; corrected: 6697.453
   Spectrum: 32; original: 10384.000; corrected: 10520.529
   Spectrum: 63; original: 8875.000; corrected: 8969.891

.. categories::

.. sourcelink::
