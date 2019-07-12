.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithms performs complete treatment of SANS data recorded with the ILL instruments D11, D22 and D33.
This high level algorithm steers the reduction and performs the full set of corrections for a given sample run; measured with one or more detector distances.
It has two operation modes: *ReduceSample* (default) and *ReduceWater* as follows:

ReduceSample
------------

This mode is used to correct the sample measurement and convert it to Q-space, producing by default the azimuthal average curve :math:`I(Q)`.

ReduceWater
-----------

This mode should be used to process water reference measurement in order to derive the relative inter-pixel sensitivity map of the detector.
This mode will produce two outputs; the regular output will contain fully corrected water run, and there will be an additional output containing the sensitivity map itself.
The sensitivity map can be saved out to a file and used for sample reductions.

Caching with ADS
----------------

This algorithm **does not** clean-up the intermediate workspaces after execution. This is done intentionally for performance reasons.
For example, once the transmission of a sample is calculated, it will be reused for further iterations of processing of the same sample at different detector distances.
As other example, once the container is processed at a certain distance, it will be reused for all the subsequent samples measured at the same distance, if the container run is the same.
The same caching is done for absorber, empty beam, container, sensitivity and mask workspaces.
The caching relies on Analysis Data Service (ADS) through naming convention by appending the relevant process name to the run number.
When multiple runs are summed, the run number of the first run is attributed to the summed workspace name.

.. categories::

.. sourcelink::
