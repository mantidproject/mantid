=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Concepts
--------
- The reference frame in :ref:`IDF <InstrumentDefinitionFile>` can now be customized in terms of setting the axis defining the 2theta sign.

Algorithms
----------

New
###

- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` performs concatenation of the workspaces into a single one by handling the sample logs merging as in :ref:`MergeRuns <algm-MergeRuns>`.

Improved
########
- :ref:`SumSpectra <algm-SumSpectra-v1>`: Fixed a bug where a wrong fallback value would be used in case of invalid values being set for min/max worspace index, and improved input validation for those properties.
- :ref:`SetUncertainties <algm-SetUncertainties-v1>` now provides a "custom" mode, which lets the user specify both an arbitrary error value whose occurences are to be replaced in the input workspace, as well as the value to replace it with.
- :ref:`LoadBBY <algm-LoadBBY-v1>` is now better at handling sample information.
- :ref:`GroupDetectors <algm-GroupDetectors-v2>` now supports workspaces with detector scans.
- :ref:`Load <algm-Load-v1>` now supports use of tilde in file paths in Python, for example Load(Filename="~/data/test.nxs", ...)
- :ref:`ConjoinWorkspaces <algm-ConjoinWorkspaces-v1>` provides option to change Y axis unit and label.
- :ref:`algm-MonteCarloAbsorption` now supports approximating the input instrument with a sparse grid of detectors enabling quick simulation of huge pixel arrays.
- :ref:`FindPeaksMD <algm-FindPeaksMD-v1>` allows now to normalize by the number of events. This can improve results for data that was originally based on histogram data which has been converted to event-mode.
- :ref:`FilterEvents <algm-FilterEvents-v1>` has refactored on splitting sample logs.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now copies units for the logs in the filtered workspaces
- :ref:`SimpleShapeMonteCarloAbsorption <algm-SimpleShapeMonteCarloAbsorption>` has been added to simplify sample environment inputs for MonteCarloAbsorption

Deprecated
##########

MD Algorithms (VATES CLI)
#########################

Performance
-----------
- Performance of UB indexing routines addressed. `:ref:`FindUBUsingLatticeParameters` running 2x faster than before.

CurveFitting
------------

New
###

- :ref:`PrimStretchedExpFT <func-PrimStretchedExpFT>` Provides the Fourier Transform of the Symmetrized Stretched Exponential Function integrated over each energy bin. Use in place of :ref:`StretchedExpFT <func-StretchedExpFT>` for fitting sample data featuring relaxation times longer than the resolution of the instrument.
- :ref:`GramCharlier <func-GramCharlier>` is a new fit function primarily for use in neutron compton scattering.

Bug fixes
#########

- :ref:`CubicSpline <func-CubicSpline>` is fixed to sort the y-values and x-values correctly.

Improved
########

Python
------

- :py:obj:`mantid.kernel.MaterialBuilder` had an issue when setting the mass density with more than one atom in the chemical formula. This is now fixed, so the number density is correctly set in :py:obj:`mantid.kernel.Material` and the cross sections correctly calculated.

Python Algorithms
#################


Python Fit Functions
####################

- A bug that makes it difficult to define and use attributes in python fit functions has been fixed.

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
