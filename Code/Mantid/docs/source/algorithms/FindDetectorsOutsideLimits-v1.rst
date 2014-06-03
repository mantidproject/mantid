.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is intended to identify detectors that are grossly over or under
counting. It reads the input workspace and identifies all histograms
with numbers of counts outside the user defined upper and lower limits.
Each spectra that fails has its spectra masked on the output workspace.
Spectra that pass the test have their data set to a positive value, 1.0.
The output workspace can be fed to :ref:`algm-MaskDetectors` to
mask the same spectra on another workspace.

ChildAlgorithms used
####################

Uses the :ref:`algm-Integration` algorithm to sum the spectra.

.. categories::
