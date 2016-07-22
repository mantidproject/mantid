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

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.7.2: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/Software/Mantid3.6.1 <http://dx.doi.org/10.5286/Software/Mantid3.7.2>`_


Changes in this version
-----------------------

* `16897 <https://github.com/mantidproject/mantid/issues/16897>`_ New IDF for TOPAZ
* `16906 <https://github.com/mantidproject/mantid/pull/16906>`_ Update live data urls for ADARA beamlines
* `16933 <https://github.com/mantidproject/mantid/pull/16933>`_ New IDF for CNCS
* `16973 <https://github.com/mantidproject/mantid/issues/16973>`_ Updated LoadInstrument calls in VisionReduction algorithm
* NOMAD geometry

Summary of impact
-----------------

+-------+--------------------------------------------------+--------------------------------------------+--------------+
| Issue | Impact                                           | Solution                                   | Side Effect  |
|       |                                                  |                                            | Probablility |
+=======+==================================================+============================================+==============+
| 16897 | TOPAZ installed ``bank21``                       | New TOPAZ idf                              | low          |
+-------+--------------------------------------------------+--------------------------------------------+--------------+
| 16906 | ADARA live data url chages                       | Modify urls in ``Facilities.xml``          | low          |
+-------+--------------------------------------------------+--------------------------------------------+--------------+
| 16933 | CNCS recalibrated detector positions             | New CNCS idf                               | low          |
+-------+--------------------------------------------------+--------------------------------------------+--------------+
| 16973 | Vision reduction                                 | Enable ``RewriteSpectraMap`` in reduction  | low          |
+-------+--------------------------------------------------+--------------------------------------------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.7.2
