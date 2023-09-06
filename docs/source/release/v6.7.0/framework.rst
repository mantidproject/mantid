=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Event Filtering
---------------

During the v6.7.0 development cycle, there were major changes to the code underlying event filtering that was originally designed `here <https://github.com/mantidproject/mantid/issues/34794>`_.
The main changes to Mantid are as follows:

- A ``TimeROI`` object that contains a list of use and ignore regions in time. The times are ``[inclusive, exclusive)``.
- When filtering/splitting logs in the ``Run`` object, the logs are copied as is and a new ``TimeROI`` object is set on the ``Run``. Values that are active during an ROI are kept, as are ones before/after each ROI. The main change is that logs no longer have fake values to mimic the filtering/splitting.
- Code should change from asking a ``TimeSeriesProperty`` for its statistics to asking the ``Run`` object for the statistics of a log because the ``Run`` is the only place that knows the information about the ``TimeROI``.
- When filtering/splitting logs, the filters are now ``[inclusive, exclusive)`` where previous behavior was ``[inclusive, inclusive]``. The previous behavior was inconsistent with how events are filtered. This change, fixes issues observed with the integrated proton charge.
- When workspaces are added, the resulting ``TimeROI`` is the union of the individual ``TimeROI`` objects. If either workspace had a ``TimeROI`` with ``TimeROI.useAll()==True``, it is assumed to be from that workspace's start-time to end-time.
- Arithmetic statistics (e.g. simple mean rather than time weighted) will be largely unchanged and are not necessarily correct.


The algorithms modified as part of this are :ref:`FilterByLogValue <algm-FilterByLogValue>`, :ref:`FilterByTime <algm-FilterByTime>`, and :ref:`FilterEvents <algm-FilterEvents>`.
The ``Sample log viewer`` has been modified to optionally show the ``TimeROI`` as an overlay on the logs.

Algorithms
----------

New features
############
- The :ref:`SplineBackground <algm-SplineBackground>` algorithm is now able to fit multiple spectra in a workspace instead of a single one.
- The beta test warning has been removed from the :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` algorithm.
- The performance of the :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` algorithm has been improved on elastic instruments with large numbers of bins.
- ``PyCIFRW`` has been added as a runtime dependency of the Mantid conda package so users no longer have to install it themselves to run the :ref:`LoadCIF <algm-LoadCIF>` algorithm.
- :ref:`CrossCorrelate <algm-CrossCorrelate>` now accepts either a list of spectrum IDs as input, or min and max spectrum IDs.

Bugfixes
############
- :ref:`LoadAndMerge <algm-LoadAndMerge>` now cleans up intermediate loaded workspaces if the ``OutputBehaviour`` was set to 'Concatenate' these inputs.
- Corrected issue in :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` where run end time was being determined incorrectly.
- Corrected issue in :ref:`FilterByTime <algm-FilterByTime>` where filter stop time was being determined incorrectly.


Beamline
--------


Bugfixes
########
- ``isMonitor`` methods will now produce an error if an index overflow occurs, to avoid unintentional behaviour.


Fit Functions
-------------

New features
############
- The ``Spin`` parameter in the :ref:`Meier function <func-Meier>` is now an attribute.
- The ``A0`` parameter in the :ref:`Redfield function <func-Redfield>` has been removed.

Bugfixes
############
- Fixed a bug that meant that when the workspace attribute of a function was changed (e.g. resolution or tabulated function) in a GUI, the function was not updated. This would lead to a crash as Mantid believed that the option was invalid.
- Fixed a bug that prevented the :ref:`UserFunction <func-UserFunction>` from being added to a composite function within custom interfaces.
- Fixed a memory leak in the wrapper for gsl derivative calculations.

Data Objects
------------

New features
############
- :ref:`LoadISISNexus <algm-LoadISISNexus>` will now load the notes from the ``.nxs`` file as a comment.
- ``EventList`` can now be filtered by ``TimeROI``.
- Added the following getter methods to ``GroupingWorkspace``:

  - ``getGroupIDs()``
  - ``getTotalGroups()``
  - ``getDetectorIDsOfGroup()``

- :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>` will now save ``GroupingWorkspace`` objects.


Geometry
--------

New features
############
- ``XMLInstrumentParameter`` now includes ``TimeROI`` in the calculation of instrument parameters.



Python
------

New features
############
- Added the ability to use strings with ``ConfigService.setLogLevel()``.

Bugfixes
############
- Updated the value returned by ``TimeSeriesProperty`` for time average mean and standard deviation. This now accounts for the last point in a log which was previously, in v6.5.0, being ignored.
- Fixed and refactored the ``rescale_flux`` method in both versions of :ref:`SANSILLReduction <algm-SANSILLReduction>` algorithms.


MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.7.0 <v6.7.0>`
