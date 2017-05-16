.. _v3.9.1:

==========================
Mantid 3.9.1 Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 3.9.0 <v3.9.0>`.

There is no common theme to the fixes contained in this patch, rather it is collection of small but
significant fixes. Please see below for the full list of changes.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.9.1: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid3.9.1 <http://dx.doi.org/10.5286/Software/Mantid3.9.1>`_


Changes in this version
-----------------------

* `18777 <https://www.github.com/mantidproject/mantid/pull/18777>`_ Add live data address for MANDI
* `18833 <https://www.github.com/mantidproject/mantid/pull/18833>`_ Added check for if nonorthogonal axes should be displayed
* `18857 <https://www.github.com/mantidproject/mantid/pull/18857>`_ Indirect Diffraction - OSIRIS diffonly interface crash
* `18865 <https://www.github.com/mantidproject/mantid/pull/18865>`_ Fix bug in reflectometry GUI
* `18875 <https://www.github.com/mantidproject/mantid/pull/18875>`_ U correction not correctly applied to viewport
* `18884 <https://www.github.com/mantidproject/mantid/pull/18884>`_ Add support for NeXus files in LoadVesuvio
* `18888 <https://www.github.com/mantidproject/mantid/pull/18888>`_ Fix LOQ Batch reduction issues
* `18891 <https://www.github.com/mantidproject/mantid/pull/18891>`_ Fix bug in gd_prtn_chrg for chunked data
* `18907 <https://www.github.com/mantidproject/mantid/pull/18907>`_ Fix zeropadding for IMAT in Facilities.XML
* `18914 <https://www.github.com/mantidproject/mantid/pull/18914>`_ Fix mass ws deletion bug
* `18915 <https://www.github.com/mantidproject/mantid/pull/18915>`_ Add missing parameter in function call in performance test
* `18926 <https://www.github.com/mantidproject/mantid/pull/18926>`_ Fix wrong detector selection when loading high angle bank user files in ISIS SANS
* `18927 <https://www.github.com/mantidproject/mantid/pull/18927>`_ Fix sum file behaviour for vesuvio diffraction
* `18952 <https://www.github.com/mantidproject/mantid/pull/18952>`_ Fix an issue in TimeSeriesProperty
* `18955 <https://www.github.com/mantidproject/mantid/pull/18955>`_ Fix crash in MonitorDlg
* `18959 <https://www.github.com/mantidproject/mantid/pull/18959>`_ Blank instrument view with U correction
* `18972 <https://www.github.com/mantidproject/mantid/pull/18972>`_ Fix reading dead time data in Muon interface
* `18979 <https://www.github.com/mantidproject/mantid/pull/18979>`_ Fix delete confirmations in workspace dock
* `18984 <https://www.github.com/mantidproject/mantid/pull/18984>`_ Abins: Correct setting numerical zero for b_tensors
* `18995 <https://www.github.com/mantidproject/mantid/pull/18995>`_ Add bank43 to TOPAZ IDF this run cycle
* `19022 <https://www.github.com/mantidproject/mantid/pull/19022>`_ Fix reduction for osiris diffspec


Summary of impact
-----------------

+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| Issue | Impact                                                                            | Solution                                    | Side Effect  |
|       |                                                                                   |                                             | Probability  |
+=======+===================================================================================+=============================================+==============+
| 18777 | Allow MANDI to use live data                                                      | Add address to facilities file              | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18833 | HKL axes now respect the non-orthogonal checkbox                                  | Add check if button is in a checked state   | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18857 | Diffraction reduction in diffonly mode does not crash fro OSIRIS                  | Only run OSIRIS-specific reduction          | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18865 | Correct behaviour when no transmission run is provided                            | Add check if runs are provided              | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18875 | Displays all of instrument when U correction applied                              | Apply U correction in appropriate all places| **medium**   |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18884 | Allows Vesuvio to Load NeXus files from current cycle                             | Use Load not LoadRaw in algorithm           | **medium**   |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18888 | Fixes batch reduction for LOQ at ISIS                                             | Checks correctly for userfile in batch run  | **medium**   |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18891 | Fixes bug in gd_prtn_chrg for chunked data                                        | Recalculate proton charge just prior to use | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18907 | Allows IMAT to use general file finding mechanism                                 | Facilities file update                      | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18914 | Avoids crash when mass deleting hundreds of workspaces in MantidPlot              | Call single algorithm in separate thread    | **medium**   |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18915 | Fixes test builds                                                                 | Fix function calls                          | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18926 | Enables ISIS SANS to select the correct bank of the detector                      | Fix interpretation of list indices in GUI   | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18927 | Allow Vesuvio to sum runs in diffraction reduction                                | Fix incorrect assignment in loop            | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18952 | Avoid out of bounds access when comparing TimeSeriesProperty objects              | Check real size of objects first            | **medium**   |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18955 | Avoid possible crash in algorithm monitor dialog                                  | Check for null pointer                      | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18959 | Fixes blank view when U correction applied                                        | Fix missing call after #18875               | **medium**   |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18972 | Loads data with/without dead-time correction as per user request                  | Clear data cache after option is updated    | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18979 | Brings back delete confirmation dialog on removing workspace                      | Propagate setting to relevant objects       | **medium**   |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18984 | Sets a minimum threshold for values of B tensor in Abins                          | Check values on load                        | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 18995 | Mantid now recognises bank 43 of TOPAZ                                            | Update IDF                                  | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 19022 | OSIRIS diffspec reduction now works                                               | Add back code in else statement             | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.9.1
