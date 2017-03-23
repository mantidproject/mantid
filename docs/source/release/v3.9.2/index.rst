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
  `doi: '


Changes in this version
-----------------------

* `19110 <https://www.github.com/mantidproject/mantid/pull/19161>`_ Fixes bug when changing group in muon analysis.
* `19129 <https://www.github.com/mantidproject/mantid/pull/19129>`_ New CNCS geometry. 
* `19161 <https://www.github.com/mantidproject/mantid/pull/19161>`_ Fixes bug when plotting after simultaneous fit.


 
Summary of impact
-----------------

+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| Issue | Impact                                                                            | Solution                                    | Side Effect  |
|       |                                                                                   |                                             | Probability  |
+=======+===================================================================================+=============================================+==============+
| 19110 |Mantid removes the previous group from the selection when fitting in muon analysis.| **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 19129 |  a new CNCS geometry is available.  | **low**      |
+-------+-----------------------------------------------------------------------------------+---------------------------------------------+--------------+
| 19161 | Simultaneous fitting produces correct fitted functions when plotted | **medium**      |
+-----------------------------------------------------------------------------------
+---------------------------------------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.9.1
