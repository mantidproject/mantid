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

- :ref:`LoadNexusLogs <algm-LoadNexusLogs>` has additional parameters to allow or block specific logs from being loaded.
- :ref:`LoadEventNexus <algm-LoadEventNexus>` now utilizes the log filter provided by `LoadNexusLogs <algm-LoadNexusLogs>`.
- New algorithm :ref:`GenerateLogbook <algm-GenerateLogbook>`, that allows creating TableWorkspace
  logbooks based on provided directory path with rawdata.
- :ref:`CompareWorkspaces <algm-CompareWorkspaces>` compares the positions of both source and sample (if extant) when property `checkInstrument` is set.
- :ref:`SetGoniometer <algm-SetGoniometer>` can now set multiple goniometers from log values instead of just the time-avereged value.
- Added the ability to specify the spectrum number in :ref:`FindPeaksAutomatic <algm-FindPeaksAutomatic>`.
- :ref:`LoadLog <algm-LoadLog>` will now detect old unsupported log files and set an appropriate explanatory string in the exception.
- :ref:`Stitch1DMany <algm-Stitch1DMany>` has additional property `IndexOfReference` to allow user to decide which of the provided workspaces should give reference for scaling

Data Objects
------------

- exposed ``geographicalAngles`` method on :py:obj:`mantid.api.SpectrumInfo`
- :ref:`Run <mantid.api.Run>` has been modified to allow multiple goniometers to be stored.
- :ref:`FileFinder <mantid.api.FileFinderImpl>` has been modified to improve search times when loading multiple runs on the same instrument.

Python
------


.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

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

- Fix problem with dictionary parameters on :ref:`SetSample <algm-SetSample>` algorithm when running from the algorithm dialog
- Fix segmentation fault when running :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` algorithm on Ubuntu without a material defined on one of the sample\environment shapes
- Fix calculation of region where scattering points are sampled in :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` when a shape is defined for the environment but not the sample

:ref:`Release 6.1.0 <v6.1.0>`
