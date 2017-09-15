.. _v3.9.2:

==========================
Mantid 3.9.2 Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 3.9.0 <v3.9.0>`.

There is no common theme to the fixes contained in this patch, rather it is collection of small but
significant fixes. Please see below for the full list of changes.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.9.2: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
  `doi: 10.5286/Software/Mantid3.9.2 <http://dx.doi.org/10.5286/Software/Mantid3.9.2>`_


Changes in this version
-----------------------

* `19110 <https://www.github.com/mantidproject/mantid/pull/19161>`__ Fixes bug when changing group in muon analysis.
* `19129 <https://www.github.com/mantidproject/mantid/pull/19129>`__ New CNCS geometry. 
* `19161 <https://www.github.com/mantidproject/mantid/pull/19161>`__ Fixes bug when plotting after simultaneous fit.


 
Summary of impact
-----------------

+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| Issue | Impact                                                                            | Solution                                    | Side Effect  |
|       |                                                                                   |                                             | Probability  |
+=======+===================================================================================+=============================================+==============+
| 19110 | Does not produce an error in Muon Analysis when fitting different groups.         |Mantid removes the previous group from the   | **low**      |
|       |                                                                                   |selection when fitting in muon analysis.     |              |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 19129 |  A new CNCS geometry is available.                                                | Added a CNCS geometry file                  | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 19161 | Simultaneous fitting produces correct fitted functions when plotted               | Changed the way parameters are reported.    | **medium**   |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.9.2
