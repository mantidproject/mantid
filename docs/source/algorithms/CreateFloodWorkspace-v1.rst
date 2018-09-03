.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm takes a measured flood run (or runs) and converts it to a workspace that can be used for flood correction of reflectometry measurements.
If `StartSpectrumIndex1 and `EndSpectrumIndex` are given then they specify a range of detectors for which the flood will be calculated, for all other
detectors it will be assumed 1. For detectors given in the `ExcludeSpectra` property the flood value will be set to a very large number that will have an effect 
of excluding these detectors from analysis.

If `CentralPixelSpectrum` is given then the integrated value of that spectrum is used to scale the flood workspace. If it is default then the
`Background` property specifies the function to use to remove the background and scale the flood values. The integration range can be specified
with the `RangeLower` and `RangeUpper` properties.

The output workspace shares the instrument with the inputs but not the logs.


.. categories::

.. sourcelink::
