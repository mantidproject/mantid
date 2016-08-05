.. _v3.7.2:

==========================
Mantid 3.7.2 Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 3.7.1 <v3.7.1>`.

The main changes are:

* Update of ``Facilities.xml`` for live data at SNS on CORELLI, NOMAD, POWGEN, and VISION.
* Updated TOPAZ instrument geometry
* Updated CNCS instrument geometry
* Fixed compilation issue of MPIAlgorithms
* Added usage tracking to interfaces
* Fixed broken beam center finder for ISIS SANS
* Made ``LoadCanSAS1D`` compliant with CanSAS standard
* Fix for saving process notes in ``SaveCanSAS1D``
* Fix for loading of 2D data with ``LoadRKH``
* Fix handling of multi-period event-type data for LARMOR
* Fix crash for ISIS SANS reduction in batch mode
* Fix initial position for beam centre finder for LARMOR
* Fix engineering diffraction interface crash

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.7.2: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/Software/Mantid3.7.2 <http://dx.doi.org/10.5286/Software/Mantid3.7.2>`_


Changes in this version
-----------------------

* `16897 <https://github.com/mantidproject/mantid/issues/16897>`_ New IDF for TOPAZ
* `16906 <https://github.com/mantidproject/mantid/pull/16906>`_ Update live data urls for ADARA beamlines
* `16933 <https://github.com/mantidproject/mantid/pull/16933>`_ New IDF for CNCS
* `16973 <https://github.com/mantidproject/mantid/issues/16973>`_ Updated LoadInstrument calls in VisionReduction algorithm
* NOMAD geometry
* `16987 <https://github.com/mantidproject/mantid/pull/16987>`_ Update VisionReduction.py
* `16946 <https://github.com/mantidproject/mantid/pull/16946>`_ Register VSI usage with UsageService
* `16575 <https://github.com/mantidproject/mantid/issues/16575>`_ Add usage tracking to interfaces and plotting tools
* `16539 <https://github.com/mantidproject/mantid/issues/16539>`_ MPI Algorithms broken in v3.7.1
* `17002 <https://github.com/mantidproject/mantid/issues/17002>`_ Loading data without Idev in LoadCanSAS causes Mantid to crash
* `16894 <https://github.com/mantidproject/mantid/issues/16894>`_ SASProcessNote not stored correctly by SaveCanSAS1D
* `16879 <https://github.com/mantidproject/mantid/issues/16879>`_ LoadRKH crashes when loading valid RKH file
* `16863 <https://github.com/mantidproject/mantid/issues/16863>`_ ISIS SANS GUI improvements
* `16787 <https://github.com/mantidproject/mantid/issues/16787>`_ ISIS SANS GUI does not load LARMOR mult-period event files
* `16759 <https://github.com/mantidproject/mantid/issues/16759>`_ Beam Center Finder seems to not work for LARMOR any longer
* `17088 <https://github.com/mantidproject/mantid/issues/17088>`_ Engineering diffraction interface crashes on new install

Summary of impact
-----------------

+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| Issue | Impact                                                       | Solution                                      | Side Effect  |
|       |                                                              |                                               | Probablility |
+=======+==============================================================+===============================================+==============+
| 16897 | TOPAZ installed ``bank21``                                   | New TOPAZ idf                                 | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16906 | ADARA live data url changes                                  | Modify urls in ``Facilities.xml``             | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16933 | CNCS recalibrated detector positions                         | New CNCS idf                                  | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16973 | Vision reduction                                             | Enable ``RewriteSpectraMap`` in reduction     | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16986 | Vision reduction fails for missing python import             | Fix import in VisionReduction.py              | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16575 | Monitor interface usage                                      | Add usage tracking to interfaces              | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16946 | Monitor VSI usage                                            | Register VSI usage with UsageService          | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16539 | MPIAlgorithms broke                                          | Fix up BroadcastWorkspace and GatherWorkspace | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 17002 | ``LoadCanSAS1D`` broken for data without Idev entry          | Make Idev optional for LoadCanSAS1D           | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16894 | SasProcessNote saved incorrectly in ``SaveCanSAS1D``         | Fix mistake in XML syntax                     | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16879 | ``LoadRKH`` does not select correct 2D loading strategy      | Make strategy selection more robust           | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16863 | ISIS SANS Batch cannot handle bad character in output        | Hedge for bad characters                      | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16863 | Beam center finder provides bad initial angle for LARMOR     | Take sign into account in angle calculation   | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16787 | Larmor multi-period event file rejected by SANS reduction    | Fix run number detection for ISIS SANS files  | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 16759 | Beam centre finder is broken in release 3.7.1                | Fix geometry description of pizza-slice mask  | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+
| 17088 | Mantid crashes because of too early enginerring prompt       | Fix having prompt appearing too early         | low          |
+-------+--------------------------------------------------------------+-----------------------------------------------+--------------+





.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.7.2
