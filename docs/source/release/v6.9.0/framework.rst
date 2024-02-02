=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- :ref:`RebinRagged <algm-RebinRagged>` has been converted from a python algorithm to a c++ algorithm. This should improve the performance of the algorithm. You can still use the Python version of :ref:`RebinRagged <algm-RebinRagged-v1>` by adding the option ``Version=1``.
- A new method was added the histogram Event data without sorting the events first, which the :ref:`Rebin <algm-Rebin>` algorithm now uses when ``PreserveEvents=False``, giving a performance improvement.
- Add support for fine-grained gitHub access tokens for authenticated calls. Can be set via ``network.github.api_token`` in Mantid.user.properties.
- Add a new option (AppendYAxisLabels) to :ref:`AppendSpectra <algm-AppendSpectra>` to append y-axis values from input workspaces to the output workspace
- Added the option to change the number of points used in the `SolidAngle` algorithm when triangulating a cylinder, which is what happens for tube detectors. The new argument is called `NumberOfCylinderSlices` and will default to `10`, which is the current number of slices used.
- Improve the error messages when calling Algorithms with either missing arguments or invalid arguments. The type of the exceptions is changed from ``RuntimeError`` to ``TypeError`` to be more consistent with python itself.
- The ``OutputWorkspace`` for the :ref:`GroupWorkspaces <algm-GroupWorkspaces>` algorithm is now allowed to be the same name as one of the ``InputWorkspaces``. In this case, the new group workspace will supplant the old workspace, and ``_1`` will be appended to the name of the old workspace.
- When the `StoreInADS` algorithm property is False, it is now optional to provide a value for `OutputWorkspace`.

Bugfixes
############
- Added warning message when ``ConvertUnits`` or ``ConvertAxisByFormula`` fails to do the conversion for certain detectors. Previously the warning was an information message.
- Fixed potential error if algorithms are still running in the background when a python script finishes.
- Fixed an issue in :ref:`Quasi <algm-BayesQuasi2>` where when using the QSE option the algorithm won't accept red files as resolution files
- Sped up the `LoadAscii` algorithm by at least one order of magnitude.
- Performance of :ref:`FilterEvents <algm-FilterEvents>` algorithm has been significantly improved for a use case with fast changing logs.
  The REF_M_40380 was acquired with a 10 kHz trigger (on/off). It has 1.3 M events and 30 M splitters. The execution time of :ref:`MRFilterCrossSections <algm-MRFilterCrossSections>`
  on this data set has been reduced from 50+ hours to 3 minutes.
- Add missing import line in BayesStretch for checking installed packages on pip.
- Improve parameter validation to check for the case of the bin width being too large when specifying bins in the Sample Transmision Calculator.
- Fixes a bug in the :ref:`Integration <algm-Integration>` algorithm caused by division by 0 when normalizing.
- Fix miscalculated Debye-Waller factor in :ref:`ComputeIncoherentDos <algm-ComputeIncoherentDos>`
- Algorithm DSFinterp, which is not registered because of a missing dependency, has been marked for deprecation.

Fit Functions
-------------

New features
############
- A new 1D peak finding Algorithm `FindPeaksConvolve` has been added to the Framework and is available in the `FitPropertyBrowser`. It has been added as an alternative to the `FindPeaks` algorithm with the intention of increasing the range of spectra we can sucessfully find peaks for.

Bugfixes
############
- Fit Function DSFinterp1DFit, which is not registered because of a missing dependency has been marked for deprecation.


Data Objects
------------

New features
############
- New C++ method ``Mantid::Kernel::Logger::isDebug()`` to allow for finding out if debug logs should even be sent to the logger for printing.
- Additional timer in `LoadEventNexus <algm-LoadEventNexus>` to give more information when profiling code using `Algorithm Profiler <https://developer.mantidproject.org/AlgorithmProfiler.html>`_
- The ``clear()`` function for the ADS now displays a warning that workspaces might get removed that are still used by one of the interfaces.

Bugfixes
############



Python
------

New features
############
- New pytest fixtures that can be used in other projects to make testing with mantid more convenient and intuitive. :ref:`pytest_fixtures`
- New algorithm available, :ref:`HFIRGoniometerIndependentBackground <algm-HFIRGoniometerIndependentBackground>`, used to generate background workspace from a 3 dimensional MDHistoWorkspace
- :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>` now accepts a background MDHistoWorkspace to be converted and subtracted from the input workspace.
- :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>` can now perform Symmetry Operations from a provided SpaceGroup, PointGroup or list of Symmetry Operations.
- added custom matplotlib backend for PyCharm to the backend cast function
- :ref:`amend_config <Amend Config>` is a new context manager available that allows you to temporarily change :ref:`Mantid User Properties <Properties File>` in a safe and efficient way.
- New function :ref:`assert_almost_equal <mantid.testing.assert_almost_equal>` to make testing equality between workspaces more convenient.
- `unique_name` and `unique_hidden_name` now available as part of :ref:`AnalysisDataServiceImpl <mantid.api.AnalysisDataServiceImpl>` to assist in creating unique names for workspaces that avoid collision.

Bugfixes
############
- Fix a deadlock that occured when using debug level logging and PythonStdoutChannel or PythonLoggingChannel


Dependencies
------------------

New features
############
- Update boost to 1.78

Bugfixes
############



MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.9.0 <v6.9.0>`