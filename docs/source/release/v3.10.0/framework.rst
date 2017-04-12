=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:


API
---

- The default multiple file limit is now made facility dependent. It is 1000 for ILL, and 100 for all the others.
- Frequency unit (GHz) included as an option to represent energy transfer.

Algorithms
----------

- Removed the optional flag ``LocationParameters`` from ``ClearInstrumentParameters``.

New
###

- :ref:`DeleteWorkspaces <algm-DeleteWorkspaces>` will delete a list of workspaces.

Improved
########

- :ref:`RawFileInfo <algm-RawFileInfo-v1>` now provides sample information.
- :ref:`SetInstrumentParameter <algm-SetInstrumentParameter-v1>` now supports also boolean parameters, and better validates the inputs.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now accepts a general TableWorkspace as the splitters workspace.  The TableWorkspace must have at least 3 columns.  The first 3 columns are for relative starting time, relative stopping time and target workspace tag for splitters, respectively.
- :ref:`GenerateEventsFilter <algm-GenerateEventsFilter-v1>` now set the unit of X-axis of the MatrixWorkspace (i.e., output splitters workspace) to second.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now generates a sample log named *splitter* of each output workspace (i.e., splitted workspace) to represent the event filter that is applied to it.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now splits all the sample logs if the input splitters are given by MatrixWorkspace or a general TableWorkspace.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now supports to filter by pulse time with input splitters in format of MatrixWorkspace and general TableWorkspace.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now sorts events before filtering.  There is no need to call :ref:`SortEvents <algm-SortEvents-v1>` before calling ``FilterEvents``.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now accept splitters from ``TableWorkspace`` and ``MatrixWrokspace`` in both relative time and epoch time.
- :ref:`FilterEvents <algm-FilterEvents-v1>` now only accept splitters from ``TableWorkspace`` and ``MatrixWrokspace`` in unit as second.
- Two new properties were added to :ref:`algm-Integration` *RangeLowerList* and *RangeUpperList* can be used to give histogram-specific integration ranges.
- :ref:`algm-FindEPP` does not output the two extra workspaces from the :ref:`algm-Fit` anymore.
- :ref:`ApplyDetailedBalance <algm-ApplyDetailedBalance>`: User can select the dynamic susceptibility versus energy or frequency.
- :ref:`MergeRuns <algm-MergeRuns>` now has a sum option and more control over failure when binning is different or sample logs do not match.
- Improved verification of IDFs

Bug Fixes
#########

- Fixed two issues with absolute rotations that affected :ref:`RotateInstrumentComponent <algm-RotateInstrumentComponent>`. Previously, setting the absolute rotation of a component to ``R`` would result in its rotation being ``parent-rotation * R * inverse(relative-parent-rotation)``.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` has been modified to allow ``EventWorkspace`` as input
- Fixed an issue where the log ``proton_charge_by_period`` was not loaded for :ref:`LoadEventNexus <algm-LoadEventNexus>`.
- Fixed an issue where :ref:`algm-MonteCarloAbsorption` would use the wavelengths from the first histogram of *InputWorkspace* only making the algorithm unusable for workspaces with varying bins.

Deprecated
##########

MD Algorithms (VATES CLI)
#########################

Performance
-----------

Following a large effort to change some of the core instrument  aspects of Mantid, we are pleased to announce very significant improvements in performance as part of the 3.10.0 release. :ref:`ConvertUnits <algm-ConvertUnits>`, for example, is running >2x times faster than possible in our last major 3.9.0 release. Amongst others, :ref:`NormaliseVanadium <algm-NormaliseVanadium>` and :ref:`MaskDetectorsInShape <algm-MaskDetectorsInShape>` now give a 2x speedup. :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis>` and :ref:`AnvredCorrection <algm-AnvredCorrection>` are examples, of a few monitored algorithms, that are demonstrating a more modest 10% improvement in speed over the last major Mantid release. Aside from granular improvements at the algorithm level, we have observed that many of the Direct Inelastic technique area workflows are 2x faster than before.

As a consequence of these changes, :ref:`CopyInstrumentParmeters <algm-CopyInstrumentParameters>`, :ref:`LoadInstrument <algm-LoadInstrument>` and several other Load Algorithms are reporting a modest slowdown. We hope to address these as part of our scheduled future work in this area.

Bugs
----

- We have fixed a bug where Mantid could crash when deleting a large number of workspaces.

CurveFitting
------------

Improved
########

- :ref:`UserFunction <func-UserFunction>` now supports :math:`erf` and :math:`erfc`.

- :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` now constrains all parameters to be non-negative which helps the fits converge faster and produces better fits.

LiveData
--------

- A new live listener for event data, `KafkaEventListener`, has been added. This is in development for the ESS and ISIS. It is only available on IBEX instruments at ISIS.

Python
------

- For multiple output parameters, python algorithms now return a ``namedtuple`` instead of a ``tuple``. Old scripts should still work,
  but one can now do

  .. code-block:: python

      results = GetEi(w)
      print(results)
      print(results.IncidentEnergy)
      print(results[0])

  This will yield:

  .. code-block:: python

      GetEi_returns(IncidentEnergy=3.0, FirstMonitorPeak=0.0, FirstMonitorIndex=0, Tzero=61.77080180287334)
      3.0
      3.0

- ``mantid.geometry.Object`` has a new method ``volume()`` which calculates the volume of the shape.
- A ``SpectraAxis`` object can now be created from Python, in a similar way to the other Axis types:

  .. code-block:: python

     ws1 = CreateSampleWorkspace()
     # Create a new axis reference
     s_axis = SpectraAxis.create(ws1)

- Fixed a bug on MDHistogramWorkspaces where passing an index larger than the size of the dimensions of the workspace to ``setSignalAt`` would crash Mantid.

Python Algorithms
#################

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
