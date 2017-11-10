.. _v3.6.1:

==========================
Mantid 3.6.1 Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 3.6.0 <v3.6.0>`.

The main changes are:

* Multiple changes to support Poco 1.4.2-1.7.2.
* Bugfix in :ref:`algm-LoadEventNexus` loading only a range of spectra.
* Bugfix in ``InternetHelper`` to correctly set headers on ``post`` requests.


Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.6: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/Software/Mantid3.6.1 <http://dx.doi.org/10.5286/Software/Mantid3.6.1>`_


Changes in this version
-----------------------

* `15366 <https://github.com/mantidproject/mantid/pull/15366>`_ Fix issues building Mantid with Poco 1.6
* `15387 <https://github.com/mantidproject/mantid/issues/15387>`_ Fix ``EQSANSLoad`` to work with Poco 1.6
* `15493 <https://github.com/mantidproject/mantid/pull/15493>`_ Undefine poco macro with the same name
* `15643 <https://github.com/mantidproject/mantid/issues/15643>`_ ``LoadEventNexus`` SpectrumMin/Max broken
* `15685 <https://github.com/mantidproject/mantid/issues/15685>`_ ``InternetHelper`` incorrectly sets both ``Transfer-Encoding: chunked`` and ``Content-Length`` headers

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
| 15643 | Loading of partial event nexus broken            | Fix logic                                  | low          |
+-------+--------------------------------------------------+--------------------------------------------+--------------+
| 15685 | Usage information not sent                       | Set only one                               | low          |
+-------+--------------------------------------------------+--------------------------------------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.6.1
