.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates a blank workspace for a given instrument with the option of
pulling in detector tables from a RAW/NeXus data file. The histogram
sizes are given by binning parameters, see :ref:`algm-Rebin`, rather
than explicit data arrays. There is also an option to set the X axis
unit.

If the DetectorTableFilename property is blank then it is assumed that a
1:1 spectra-mapping is required and the workspace will have the same
number of histograms as detectors in the instrument (not including
monitors)

.. categories::

.. sourcelink::
