============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- Minimum and maximum thresholds for calculated efficiency are now exposed to the users of :ref:`SANSILLReduction <algm-SANSILLReduction>` algorithms (version 1 and 2), as well as the higher-level algorithms calling them: :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, :ref:`SANSILLMultiProcess <algm-SANSILLMultiProcess>`, and :ref:`SANSILLParameterScan <algm-SANSILLParameterScan>`. This avoids the issue of calculated efficiency surpassing the default limit (0 and 2, respectively) and being set to :math:`-infinity`.
- D16B now has two monitors in the scanned variables, instead of one. Searching for monitor indices in the scanned variables table is performed for each file in :ref:`LoadILLSANS <algm-LoadILLSANS>`.
- :ref:`SANSILLReduction <algm-SANSILLReduction-v2>` can now process empty container data coming from a scan measurement.

Bugfixes
--------
- :ref:`SANSILLParameterScan <algm-SANSILLParameterScan>` can now perform container and absorber corrections and communicates correctly with the underlying :ref:`SANSILLReduction <algm-SANSILLReduction>` algorithm.
- Fixed a bug where the ``Save File`` input on the :ref:`Sum Runs tab <ISIS_SANS_Sum_Runs_Tab-ref>` in the :ref:`ISIS SANS GUI <ISIS_Sans_interface_contents>` would stop auto-populating if you simply clicked the input or pressed enter without making any text changes.

:ref:`Release 6.7.0 <v6.7.0>`
