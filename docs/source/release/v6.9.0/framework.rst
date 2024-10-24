=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New features
############
- The :ref:`RebinRagged <algm-RebinRagged>` algorithm has been converted from a Python algorithm to a C++ algorithm. This should improve the performance of the algorithm. You can still use the Python version of :ref:`RebinRagged <algm-RebinRagged-v1>` by adding the option ``Version=1``.
- The ``PreserveEvents=False`` option in the :ref:`Rebin <algm-Rebin>` algorithm allows you to skip the sorting of events before rebinning. This gives a performance improvement.
- Added support for fine-grained GitHub access tokens for authenticated calls. These tokens can be set via the ``network.github.api_token`` option in the ``Mantid.user.properties`` file.
- Added the ``AppendYAxisLabels`` option to the :ref:`AppendSpectra <algm-AppendSpectra>` algorithm. This will append y-axis values from input workspaces to the output workspace when True.
- Added the option to change the number of points used in the :ref:`SolidAngle <algm-SolidAngle>` algorithm when triangulating a cylinder, which is what happens for tube detectors. The new argument is called ``NumberOfCylinderSlices`` and will default to ``10``, which is the current number of slices used.
- Improved the error messages when calling Algorithms with either missing arguments or invalid arguments. The type of the exceptions is changed from ``RuntimeError`` to ``TypeError`` to be more consistent with the convention in Python.
- The ``OutputWorkspace`` for the :ref:`GroupWorkspaces <algm-GroupWorkspaces>` algorithm is now allowed to be the same name as one of the ``InputWorkspaces``. In this case, the new group workspace will supplant the old workspace, and ``_1`` will be appended to the name of the old workspace.
- When the ``StoreInADS`` algorithm property is False, it is now optional to provide a value for ``OutputWorkspace``.

Bugfixes
############
- Added warning message when ``ConvertUnits`` or ``ConvertAxisByFormula`` fails to do the conversion for certain detectors. Previously the warning was an information message.
- Fixed an error if algorithms are still running in the background when a python script finishes.
- The :ref:`LoadAscii <algm-LoadAscii>` algorithm has been sped up by at least one order of magnitude.
- The performance of the :ref:`FilterEvents <algm-FilterEvents>` algorithm has been significantly improved for a use case with fast changing logs. The ``REF_M_40380`` was acquired with a 10 kHz trigger (on/off). It has 1.3 M events and 30 M splitters. The execution time of the :ref:`MRFilterCrossSections <algm-MRFilterCrossSections>` algorithm on this data has been reduced from 50+ hours to 3 minutes.
- Improved parameter validation to check for the case of the bin width being too large when specifying bins in the :ref:`sample_transmission_calculator`.
- Fixed a bug in the :ref:`Integration <algm-Integration>` algorithm caused by division by 0 when normalizing.
- Fixed a miscalculated Debye-Waller factor in the :ref:`ComputeIncoherentDos <algm-ComputeIncoherentDos>` algorithm.
- The ``DSFinterp`` algorithm has been deprecated. It was not being registered due to a missing dependency.

Fit Functions
-------------

New features
############
- A new 1D peak finding Algorithm :ref:`FindPeaksConvolve <algm-FindPeaksConvolve>` has been added to the Framework and is available in the ``FitPropertyBrowser``. It has been added as an alternative to the :ref:`FindPeaks <algm-FindPeaks>` algorithm with the intention of improving peak finding.

Bugfixes
############
- The ``DSFinterp1DFit`` fit function has been deprecated. It was not being registered due to a missing dependency.

Data Handling
-------------

Bugfixes
########
- Fixed a bug where ``FilteredTimeSeriesProperty::timeAverageValue`` would return an incorrect value for datasets with only one data point in the filter.

Data Objects
------------

New features
############
- The ``AnalysisDataService::clear()`` function now displays a warning that workspaces might be removed even though they are still used by one of the interfaces.


Python
------

New features
############
- Added new pytest fixtures that can be used in other projects to make testing with mantid more convenient and intuitive. :ref:`pytest_fixtures`
- Added the :ref:`HFIRGoniometerIndependentBackground <algm-HFIRGoniometerIndependentBackground>` algorithm for generating a background workspace from a 3 dimensional MDHistoWorkspace.
- The :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>` algorithm now accepts a background MDHistoWorkspace - to be converted and subtracted from the input workspace.
- The :ref:`ConvertWANDSCDtoQ <algm-ConvertWANDSCDtoQ>` algorithm can now perform Symmetry Operations from a provided SpaceGroup, PointGroup or list of Symmetry Operations.
- Added a custom matplotlib backend for PyCharm to the backend cast function.
- Added a new context manager, :ref:`amend_config <Amend Config>`, that allows you to temporarily change :ref:`Mantid User Properties <Properties File>` in a safe and efficient way.
- Added a new testing function :ref:`assert_almost_equal <mantid.testing.assert_almost_equal>` to make testing equality between workspaces more convenient.
- Added the ``unique_name`` and ``unique_hidden_name`` function to the :ref:`AnalysisDataServiceImpl <mantid.api.AnalysisDataServiceImpl>` class to assist in creating unique workspace names.

Bugfixes
############
- Fixed a deadlock that occured when using debug level logging and PythonStdoutChannel or PythonLoggingChannel.


Dependencies
------------------

New features
############
- Updated boost to 1.78.

See :doc:`mantidworkbench`.
:ref:`Release 6.9.0 <v6.9.0>`
