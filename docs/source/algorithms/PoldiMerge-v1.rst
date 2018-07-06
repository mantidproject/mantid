.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

PoldiMerge takes a list of workspace names and adds the counts,
resulting in a new workspace. The difference to :ref:`algm-Plus` is that it performs
some POLDI-specific tests that determine whether merging those files is
sensible or not. The following requirements have to be fulfilled:

-  The time-binning (x-data) of all workspaces must match (offset as
   well as width of time bins)
-  These quantities from the sample log:

   -  Position of the sample table (x, y and z)
   -  Rotation speed of the chopper

The algorithm does not perform partial summation - if any of the
workspaces does not fulfill the criteria, the intermediate result is
discarded.

Usage
-----

.. include:: ../usagedata-note.txt

This small usage example merges two compatible POLDI-files which have been loaded before.

.. testcode:: ExMergeSilicon
    
    # Load the first data file and the correct instrument
    raw_6903 = LoadSINQFile(Filename = "poldi2013n006903.hdf", Instrument = "POLDI")
    LoadInstrument(raw_6903, RewriteSpectraMap=True, InstrumentName = "POLDI")

    # Use Integration and SumSpectra to sum all counts in the spectrum.
    # The data must be converted to histogram data for Integration to work.
    histo_6903 = ConvertToHistogram(raw_6903)
    spectra_6903 = Integration(histo_6903)
    total_6903 = SumSpectra(spectra_6903)
    
    # The result has one spectrum with one bin, which contains the total counts.
    counts_6903 = int(total_6903.dataY(0)[0])
    print("6903 contains a total of {} counts.".format(counts_6903))
    
    # The same with the second data file
    raw_6904 = LoadSINQFile(Filename = "poldi2013n006904.hdf", Instrument = "POLDI")
    LoadInstrument(raw_6904, RewriteSpectraMap=True, InstrumentName = "POLDI")
    histo_6904 = ConvertToHistogram(raw_6904)
    spectra_6904 = Integration(histo_6904)
    total_6904 = SumSpectra(spectra_6904)
    
    counts_6904 = int(total_6904.dataY(0)[0])
    print("6904 contains a total of {} counts.".format(counts_6904))

    # Now PoldiMerge is used to merge the two raw spectra by supplying a list of workspace names.
    raw_summed = PoldiMerge("raw_6903,raw_6904")

    # The merged data is integrated as well.
    histo_summed = ConvertToHistogram(raw_summed)
    spectra_summed = Integration(histo_summed)
    total_summed = SumSpectra(spectra_summed)

    print("6903+6904 contains a total of {} counts.".format(int(total_summed.dataY(0)[0])))
    print("Summing the counts of the single data files leads to {} counts.".format(int(counts_6903 + counts_6904)))

Output:

.. testoutput:: ExMergeSilicon

   6903 contains a total of 769269 counts.
   6904 contains a total of 766777 counts.
   6903+6904 contains a total of 1536046 counts.
   Summing the counts of the single data files leads to 1536046 counts.


.. categories::

.. sourcelink::
