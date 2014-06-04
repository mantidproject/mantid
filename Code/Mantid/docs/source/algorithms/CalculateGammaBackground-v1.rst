.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is currently used by the Vesuvio spectrometer at ISIS to
correct for background produced by photons that are produced when the
neutrons are absorbed by the shielding on the instrument. It only
corrects the forward scattering detector banks.

Two workspaces are produced: the calculated background and a corrected
workspace where the input workspace has been corrected by the
background. The background is computed by a simple simulation of the
expected count across all of the foils. The corrected workspace counts
are computed by calculating a ratio of the expected counts at the
detector to the integrated foil counts (:math:`\beta`) and then the
final corrected count rate :math:`\displaystyle c_f` is defined as
:math:`\displaystyle c_f = c_i - \beta c_b`.

.. categories::
