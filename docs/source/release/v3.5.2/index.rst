.. _v3.5.2:

==========================
Mantid 3.5.2 Release Notes
==========================

.. contents:: Table of Contents
   :local:


This is a patch of `version 3.5.1 <http://www.mantidproject.org/Release_Notes_3.5.1>`_ with changes to support Poco 1.4.2-1.7.2.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.5: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/Software/Mantid3.5.1 <http://dx.doi.org/10.5286/Software/Mantid3.5.1>`_


Changes in this version
-----------------------

* `15366 <https://github.com/mantidproject/mantid/pull/15366>`_ Fix issues building Mantid with Poco 1.6
* `15387 <https://github.com/mantidproject/mantid/issues/15387>`_ Fix ``EQSANSLoad`` to work with Poco 1.6
* `15493 <https://github.com/mantidproject/mantid/pull/15493>`_ Undefine poco macro with the same name

Summary of impact
-----------------

+-------+--------------------------------------------------+--------------------------------------------+--------------+
| Issue | Impact                                           | Solution                                   | Side Effect  |
|       |                                                  |                                            | Probablility |
+=======+==================================================+============================================+==============+
| 15366 | ``Poco::StringTokenizer`` changed implementation | Create ``Mantid::Kernel::StringTokenizer`` | medium       |
+-------+--------------------------------------------------+--------------------------------------------+--------------+
| 15387 | ``EQSANSLoad`` returned inconsistent results     | Changed rounding of floats                 | low          |
+-------+--------------------------------------------------+--------------------------------------------+--------------+
| 15493 | Poco reimplemented macro                         | Precompiler macro changes                  | low          |
+-------+--------------------------------------------------+--------------------------------------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.5.2
