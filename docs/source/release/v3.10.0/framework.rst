=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

API
---

- Frequency unit (GHz) included as an option to represent energy transfer.

Algorithms
----------

New
###

- :ref:`DeleteWorkspaces <algm-DeleteWorkspaces>` will delete a list of workspaces.

Improved
########

- :ref`RawFileInfo <algm-RawFileInfo-v1>` now provides sample information.
- :ref`SetInstrumentParameter <algm-SetInstrumentParameter-v1>` now supports also boolean parameters, and better validates the inputs.
- Two new properties were added to :ref:`algm-Integration`: *RangeLowerList* and *RangeUpperList* can be used to give histogram-specific integration ranges.

Bug Fixes
#########

- Fixed two issues with absolute rotations that affected :ref:`RotateInstrumentComponent <algm-RotateInstrumentComponent>`. Previously, setting the absolute rotation of a component to ``R`` would result in its rotation being ``parent-rotation * R * inverse(relative-parent-rotation)``.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` has been modified to allow `EventWorkspace` as input
- Fixed an issue where the log `proton_charge_by_period` was not loaded for `LoadEventNexus <algm-LoadEventNexus>`.


Deprecated
##########

MD Algorithms (VATES CLI)
#########################

Performance
-----------

Bugs
----

- We have fixed a bug where Mantid could crash when deleteing a large number of workspaces.

CurveFitting
------------

Improved
########

- :ref:`UserFunction <func-UserFunction>` now supports :math:`erf` and :math:`erfc`.

- :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` now constrains all parameters to be non-negative which helps the fits converge faster and produces better fits.

Python
------

- For multiple output parameters, python algorithms now return a `namedtuple` instead of a tuple. Old scripts should still work,
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



Python Algorithms
#################

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
