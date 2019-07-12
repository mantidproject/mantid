============
MuSR Changes
============

.. image:: ../../images/MuonFrequencyDomainAnalysisHome.png
   :align: right
   :height: 400px

.. image:: ../../images/MuonALC.png
   :align: right
   :height: 300px

.. image:: ../../images/MuonAnalysisv2.png
   :align: right
   :height: 400px

.. image:: ../../images/elemental_analysis.png
   :align: right
   :height: 300px

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


:ref:`Release 4.1.0 <v4.1.0>`

New
###

* :ref:`Frequency Domain Analysis <Frequency_Domain_Analysis_2-ref>` GUI added to workbench.
* :ref:`Muon ALC GUI <MuonALC-ref>` added to workbench.
* Added phase tab for calculating :ref:`phase tables <algm-CalMuonDetectorPhases>` and :ref:`PhaseQuad <algm-PhaseQuad>`
  workspaces to Frequency Domain Analysis GUI.
* :ref:`Muon Analysis v2 <MuonAnalysis_2-ref>` added to MantidPlot and workbench.
* :ref:`Elemental analysis <Muon_Elemental_Analysis-ref>` interface added to workbench.

Improvements
############

* Phase table and phase Quad options from frequency domain transform tab moved to phase calculations tab.
* The new interface is designed to better handle multiple runs while being more intuitive to use.
* Significant increase in the number of tests for the code, which means greater stability of the interface, a vast reduction
  in the number of hard crashes and a much more pleasant and productive experience overall.
* Added the possibility of choosing the order of a sequential fit.
* When plotting peaks in the Elemental Analysis interface, lines for different elements will appear in different colours.
* The Frequency Domain Analysis GUI now allows users to load and group detectors.
* The tabs for each of the interfaces can be detached and turned into separate windows, making for greater customizability
  of the interface.
* When fitting data in the Muon Analysis v2 interface it is possible to customize the name assigned to the function
  and workspace group.
* The resulting workspaces (in the `Result` tab of Muon Analysis) will appear sorted by fitting function.
* In the `Result` tab of Muon Analysis it is possible to search for specific log values, exclude specific value or
  display only selected ones.


Bug Fixes
#########

* Muon Analysis (original) no longer crashes when `TF Asymmetry` mode is activated.
* Muon Analysis (original) can now produce results tables when columns contain both ranges and single values.
* Elemental analysis no longer crashes when it is re-opened an a plot is made.
