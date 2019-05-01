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

Improvements
############

- :ref:`LoadEventNexus <algm-LoadEventNexus>` has an additional option `LoadNexusInstrumentXML` = `{Default, True}`,  which controls whether or not the embedded instrument definition is read from the NeXus file.
- The numerical integration absorption algorithms (:ref:`AbsorptionCorrection <algm-AbsorptionCorrection>`, :ref:`CuboidGaugeVolumeAbsorption <algm-CuboidGaugeVolumeAbsorption>`, :ref:`CylinderAbsorption <algm-CylinderAbsorption>`, :ref:`FlatPlateAbsorption <algm-FlatPlateAbsorption>`) have been modified to use a more numerically stable method for performing the integration, `pairwise summation <https://en.wikipedia.org/wiki/Pairwise_summation>`_.
- Improved support for thin-walled hollow cylinder shapes in :ref:`algm-MonteCarloAbsorption`
- Support has been added for negative indexing of :ref:`WorkspaceGroups <WorkspaceGroup>`.
  Try :code:`ws_group[-1]` to get the last workspace in the WorkspaceGroup :code:`ws_group`.
- :ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` is able to accept any `MatrixWorkspace`, as long as it has run objects loaded from `LoadNexusLogs <algm-LoadNexusLogs>`, other than `EventWorkspace`.
- :ref:`AbsorptionCorrection <algm-AbsorptionCorrection>` has a new property `ScatterFrom` which allows for calculating the correction for the other components (i.e. container and environment)
- Prevent an error due to the locale settings which may appear when reading, for instance, the incident energy Ei value from the logs in :ref:`ConvertUnits <algm-ConvertUnits>` and many other algorithms.
- :code:`indices` and :code:`slicepoint` options have been added to :ref:`mantid.plots <mantid.plots>` to allow selection of which plane to plot from an MDHistoWorkspace
- :ref:`Pseudo-Voigt <func-PseudoVoigt>` has been modified to be more in line with FULLPROF and GSAS.

Removed
#######

- The deprecated version 1 of the `FindEPP` algorithm has been removed. Use :ref:`FindEPP-v2 <algm-FindEPP>` instead.

Data Objects
------------

Python
------

:ref:`Release 4.1.0 <v4.1.0>`
