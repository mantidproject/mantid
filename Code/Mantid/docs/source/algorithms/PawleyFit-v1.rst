.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm performs a fit of lattice parameters using the principle approach described in a paper by Pawley. In this approach the reflection positions are calculated from lattice parameters and the reflection's Miller indices (:math:`hkl`), while the other profile parameters for each peak are freely refined.

PawleyFit requires a MatrixWorkspace with at least one spectrum in terms of either :math:`d` or :math:`Q`, the index of the spectrum can be supplied to the algorithm as a parameter. Furthermore, the range which is used for refinement can be changed by setting the corresponding properties.

In addition, a TableWorkspace with information about the reflections that are found in the spectrum must be passed as well. There must be four columns with the captions "HKL", "d", "FWHM (rel.)" and "Intensity". The HKL column can be supplied either as V3D or as a string with 3 numbers separated by space, comma or semi-colon and possibly surrounded by square brackets.

Usage
-----

.. include:: ../usagedata-note.txt

This small usage example merges two compatible POLDI-files which have been loaded before.

.. testcode:: ExMergeSilicon

    # Load the first data file and the correct instrument
    raw_6903 = LoadSINQFile(Filename = "poldi2013n006903.hdf", Instrument = "POLDI")
    LoadInstrument(raw_6903, InstrumentName = "POLDI")

    # Use Integration and SumSpectra to sum all counts in the spectrum.
    # The data must be converted to histogram data for Integration to work.
    histo_6903 = ConvertToHistogram(raw_6903)
    spectra_6903 = Integration(histo_6903)
    total_6903 = SumSpectra(spectra_6903)
    
    # The result has one spectrum with one bin, which contains the total counts.
    counts_6903 = int(total_6903.dataY(0)[0])
    print "6903 contains a total of", counts_6903, "counts."
    
    # The same with the second data file
    raw_6904 = LoadSINQFile(Filename = "poldi2013n006904.hdf", Instrument = "POLDI")
    LoadInstrument(raw_6904, InstrumentName = "POLDI")
    histo_6904 = ConvertToHistogram(raw_6904)
    spectra_6904 = Integration(histo_6904)
    total_6904 = SumSpectra(spectra_6904)
    
    counts_6904 = int(total_6904.dataY(0)[0])
    print "6904 contains a total of", counts_6904, "counts."

    # Now PoldiMerge is used to merge the two raw spectra by supplying a list of workspace names.
    raw_summed = PoldiMerge("raw_6903,raw_6904")

    # The merged data is integrated as well.
    histo_summed = ConvertToHistogram(raw_summed)
    spectra_summed = Integration(histo_summed)
    total_summed = SumSpectra(spectra_summed)

    print "6903+6904 contains a total of", int(total_summed.dataY(0)[0]), "counts."
    print "Summing the counts of the single data files leads to", counts_6903 + counts_6904, "counts."

Output:

.. testoutput:: ExMergeSilicon

   6903 contains a total of 769269 counts.
   6904 contains a total of 766777 counts.
   6903+6904 contains a total of 1536046 counts.
   Summing the counts of the single data files leads to 1536046 counts.


.. categories::
