
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

   # Algorithm inputs
   transmission_args = {
      "Runs": '58386-58396',
      "EmptyRuns": '57580-57603',
      "Grouping": "SumOfAllRuns",
      "Target": "Energy",
      "Rebin": True,
      "RebinParameters": "0.6,-0.03,1.e6",
      "CalculateXS": False,
      "InvertMonitors": False,
      "SmoothIncidentSpectrum": False,
      "OutputWorkspace": "ws_transmission",
   }

   # Run algorithm
   VesuvioTransmission(**transmission_args)

   # Test output
   print("The VesuvioTransmission ran successfully")

Output:

.. testoutput:: VesuvioTransmissionExample
  :options: +NORMALIZE_WHITESPACE

   The VesuvioTransmission ran successfully

.. categories::

.. sourcelink::
