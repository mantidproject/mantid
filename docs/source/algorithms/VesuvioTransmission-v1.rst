
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Evaluates the transmission spectrum on the VESUVIO spectrometer for measured sample and empty run numbers.

Usage
-----

**Example - VesuvioTransmission**

.. testcode:: VesuvioTransmissionExample

   # Run algorithm
   VesuvioTransmission(
      OutputWorkspace = "ws_transmission",
      Runs = "58386-58396",
      EmptyRuns = "57580-57603",
      Grouping = "SumOfAllRuns",
      Target = "Energy",
      Rebin = True,
      RebinParameters = [0.6,-0.03,1.e6],
      CalculateXS = False,
      InvertMonitors = False,
      SmoothIncidentSpectrum = False,
   )

   # Test output
   print(f"The output workspace contains {mtd['ws_transmission'].getNumberHistograms()} spectrum and {mtd['ws_transmission'].blocksize()} bins.")

Output:

.. testoutput:: VesuvioTransmissionExample
  :options: +NORMALIZE_WHITESPACE

   The output workspace contains 1 spectrum and 484 bins.

.. categories::

.. sourcelink::
