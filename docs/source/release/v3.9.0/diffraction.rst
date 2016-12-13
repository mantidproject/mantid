===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Crystal Improvements
--------------------
:ref:`algm-FindUBUsingLatticeParameters` will now return an oriented lattice even when the number of peaks used is very low.

:ref:`algm-FindUBUsingLatticeParameters` has a new option to fix lattice parameters. This will find an orientation, but without optimisation between indexed HKLs and q vectors.

:ref:`algm-CreateGroupingWorkspace` has a new option to create one group of 4 columns of SNAP detectors and another with the remaining 2 columns. This grouping is used frequently in their reduction.

:ref:`algm-IntegratePeaksMD` now removes the top 1% of the background events so that intensity spikes near the edges are removed.

:ref:`algm-IntegrateEllipsoids` has an new option, AdaptiveQMultiplier, for the radius to vary as a function of the modulus of Q. If the AdaptiveQBackground option is set to True, the background radius also changes.  These are the same as the adaptive options in :ref:`algm-IntegratePeaksMD`.

Engineering Diffraction
-----------------------

Powder Diffraction
------------------

:ref:`algm-SNSPowderReduction` had an error in logic of subtracting the vanadium background. It was not being subtracted when ``PreserveEvents=True``.

:ref:`algm-PDLoadCharacterizations` and
:ref:`algm-PDDetermineCharacterizations` have been upgraded to support
sample container specific information, as well as additional
information about the empty sample environment and instrument.

:ref:`algm-SetDetScale` has a new option, DetScaleFile, to input a text file with each line containing the detector number and scale factor for that detector.  These scales will be used in SaveHKL and AnvredCorrection.  If scales for a detector are given in both the DetScaleList text string and the DetScaleFile file, the values from the text string will be used.

:ref:`algm-SNAPReduce` is new to mantid, but not for SNAP
users. Adding the algorithm to mantid installations will reduce the
amount of issues that SNAP users will encounter trying to reduce their
data.

Full list of `diffraction <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Diffraction%22>`_
and
`imaging <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Imaging%22>`_ changes on GitHub.
