============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Algorithms
----------

New
###

- :ref:`ApplyMuonDetectorGrouping <algm-ApplyMuonDetectorGrouping>` added to allow scripting of the Muon Analysis GUI workflow. Applies grouping counts/asymmetry to muon data and stores the result in the ADS.
- :ref:`ApplyMuonDetectorGroupPairing <algm-ApplyMuonDetectorGroupPairing>` added to allow scripting of the Muon Analysis GUI workflow. Applies a group pairing asymmetry calculation to muon data and stores the result in the ADS.
- :ref:`LoadAndApplyMuonDetectorGrouping <algm-LoadAndApplyMuonDetectorGrouping>` added to allow scripting of the Muon Analysis GUI workflow. The grouping/pairing information is loaded from an XML format file, which can be produced through the muon analysis GUI via the 'Save Grouping' button. Replicates the `Load Grouping` button of the grouping tab, adds workspaces to the ADS.

:ref:`Release 3.14.0 <v3.14.0>`
