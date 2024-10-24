============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New features
------------
- The NXcanSAS format is now supported in Mantid using the :ref:`SaveNXcanSAS <algm-SaveNXcanSAS>` and :ref:`LoadNXcansSAS <algm-LoadNXcanSAS>` algorithms.


Bug Fixes
---------

- :ref:`Q1D <algm-Q1D>`: Fix incorrect extra length for gravity correction to :math:`(L+L_{extra})^2 - L_{extra}^2`
- Select the IDF based on the workspace which is being loaded. Previously the IDFs were hardcoded. This solves a bug were the wrong IDF was sometimes used.


`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+SANS%22>`__
