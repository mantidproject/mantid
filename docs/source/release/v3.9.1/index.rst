.. _v3.9.1:

==========================
Mantid 3.9.1 Release Notes
==========================

.. contents:: Table of Contents
   :local:

This is a patch release that corrects some significant issues since :ref:`version 3.9.0 <v3.9.0>`.

There is no common theme to the fixes contained in this patch, rather it is collection of small but significant fixes. Please see below
for the full list of changes.

Citation
--------

Please cite any usage of Mantid as follows:

- *Mantid 3.9.1: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*.
`doi: 10.5286/Software/Mantid3.9.1 <http://dx.doi.org/10.5286/Software/Mantid3.9.1>`_


Changes in this version
-----------------------

* `18884 <https://api.github.com/repos/mantidproject/mantid/pulls/18884>`_ Add support for NeXus files in LoadVesuvio
* `18888 <https://api.github.com/repos/mantidproject/mantid/pulls/18888>`_ Fix LOQ Batch reduction issues
* `18889 <https://api.github.com/repos/mantidproject/mantid/pulls/18889>`_ Workspace dock deletion bugs
* `18914 <https://api.github.com/repos/mantidproject/mantid/pulls/18914>`_ Fix mass ws deletion bug
* `18926 <https://api.github.com/repos/mantidproject/mantid/pulls/18926>`_ Fix wrong detector selection when loading high angle bank user files in ISIS SANS
* `18927 <https://api.github.com/repos/mantidproject/mantid/pulls/18927>`_ Fix sum file behaviour for vesuvio diffraction

Summary of impact
-----------------

+-------+-----------------------------------------------------------------------------------+----------+--------------+
| Issue | Impact                                                                            | Solution | Side Effect  |
|       |                                                                                   |          | Probability  |
+=======+===================================================================================+==========+==============+
| 18884 | Add support for NeXus files in LoadVesuvio                                        |          | **unknown**  |
+-------+-----------------------------------------------------------------------------------+----------+--------------+
| 18888 | Fix LOQ Batch reduction issues                                                    |          | **unknown**  |
+-------+-----------------------------------------------------------------------------------+----------+--------------+
| 18889 | Workspace dock deletion bugs                                                      |          | **unknown**  |
+-------+-----------------------------------------------------------------------------------+----------+--------------+
| 18914 | Fix mass ws deletion bug                                                          |          | **unknown**  |
+-------+-----------------------------------------------------------------------------------+----------+--------------+
| 18926 | Fix wrong detector selection when loading high angle bank user files in ISIS SANS |          | **unknown**  |
+-------+-----------------------------------------------------------------------------------+----------+--------------+
| 18927 | Fix sum file behaviour for vesuvio diffraction                                    |          | **unknown**  |
+-------+-----------------------------------------------------------------------------------+----------+--------------+

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v3.9.1
