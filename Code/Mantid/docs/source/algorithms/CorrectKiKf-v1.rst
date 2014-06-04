.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs ki / kf multiplication, in order to transform differential
scattering cross section into dynamic structure factor. Both Ei and Ef
must be positive. However, if this requirement is not met, it will give
an error only if the data is not 0. This allows applying the algorithms
to rebinned data, where one can rebin in Direct EMode to energies higher
than EFixed. If no value is defined for EFixed, the algorithm will try
to find Ei in the workspace properties for direct geometry spectrometry,
or in the instrument definition, for indirect geometry spectrometry.
Algorithm is event aware. TOF events will be changed to weighted events.

.. categories::
