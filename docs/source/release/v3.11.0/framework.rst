=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###

- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` performs concatenation of the workspaces into a single one by handling the sample logs merging as in :ref:`MergeRuns <algm-MergeRuns>`.

Improved
########

- :ref:`SetUncertainties <algm-SetUncertainties-v1>` now provides a "custom" mode, which lets the user specify both an arbitrary error value whose occurences are to be replaced in the input workspace, as well as the value to replace it with.
- :ref:`LoadBBY <algm-LoadBBY-v1>` is now better at handling sample information. 
- :ref:`GroupDetectors <algm-GroupDetectors-v2>` now supports workspaces with detector scans.

Deprecated
##########

MD Algorithms (VATES CLI)
#########################

Performance
-----------

CurveFitting
------------

Improved
########

Python
------

Python Algorithms
#################

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
