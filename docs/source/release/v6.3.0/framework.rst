=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------
- Introducing a naming convention for algorithms, and *deprecated aliases* as the preferred method for renaming a C++ or Python algorithm.
- Enabling deprecation of Python algorithms; instructions on how to deprecate a C++ or Python algorithm in the developer documentation.

New Features
############
- Added a :ref:`Power Law <func-PowerLaw>` function to General Fit Functions.

Improvements
############

- :ref:`SaveAscii <algm-SaveAscii>` and :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>` have a new property OneSpectrumPerFile, controlling whether or not to save each spectrum in an individual file or all the spectra into a single file.
- :ref:`GenerateLogbook <algm-GenerateLogbook>` now allows to perform binary operations even when certain entries do not exist, e.g. to create a string with all polarisation orientations contained in a collection of data files.
- Event nexuses produced at ILL can now be loaded using :ref:`LoadEventNexus <algm-LoadEventNexus>`.
- :ref:`Rebin <algm-Rebin>` now has an option for binning with reverse logarithmic and inverse power bins.
- :ref:`SetSampleFromLogs <algm-SetSampleFromLogs>` will now fail if the resulting sample shape has a volume of 0.
- :ref:`SetSample <algm-SetSample>` can now load sample environment XML files from any directory using ``SetSample(ws, Environment={'Name': 'NameOfXMLFile', 'Path':'/path/to/file/'})``.
- An importance sampling option has been added to :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` so that it handles spikes in the structure factor S(Q) better
- Added parameter to :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` to control number of attempts to generate initial scatter point

Bugfixes
########

- Fix bug in :ref:`Integration <algm-Integration>` when using UsePartialBinsOption with integration limits that are either equal or close together

Fit Functions
-------------
New Features
############
- Fixed a bug in :ref:`UserFunction<func-UserFunction>` where the view would not be updated with the parameters in the formula entered.

Data Objects
------------

Geometry
----------
- add additional unit test for Rasterize class.

Python
------

- `isGroup` can now be used to determine if a workspace/table workspace is a grouped workspace object.
- `createChildAlgorithm` now accepts property keyword arguments to set the child algorithm's properties during creation:

  -  Existing arguments, such as version, start and end progress...etc. are unaffected by this change.
  -  E.g. `createChildAlgorithm("CreateSampleWorkspace", version=1, XUnit="Wavelength")`

Installation
------------

MantidWorkbench
---------------

See :doc:`mantidworkbench`.

SliceViewer
-----------

Improvements
############

Bugfixes
########
- Fix out-of-range error when trying to access the projection matrix in sliceviewer for a workspace with a non-Q axis before other Q axes.
- For MDHisto workspaces get the projection matrix from the basis vectors on the workspace rather than search for the W_MATRIX log.


- Fixed bug in :ref:`Run <Run>` goniometer when using :ref:`algm-Plus`.

:ref:`Release 6.3.0 <v6.3.0>`
