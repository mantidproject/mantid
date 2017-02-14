=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Algorithms
----------

New
###


Improved
########


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

- :ref:`SplineInterpolation <algm-SplineInterpolation>` treats the input WorkspacesToInterpolate containing two points if desired: the user has the option to do a linear interpolation. Furthermore, a new option ReferenceWorkspace defines from which InputWorkspace properties will be copied. Sorting improved for workspaces containing point data, i.e. not only x values will be sorted like for histogram workspaces.

Python
------

Python Algorithms
#################

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
