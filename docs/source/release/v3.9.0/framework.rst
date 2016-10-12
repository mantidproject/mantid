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

Python
------

Python Algorithms
#################


MatchPeaks is a new Python algorithm that transforms a MatrixWorkspace: while keeping the x-values, y-values and e-values can be shifted in order to newly align its peak positions of each spectrum. The algorithm cannot take into account multiple peaks present in single spectrum. It's use is to centre each spectrum or to shift spectra according to peak positions of a compatible, second workspace. Workspaces are compatible by means of number of spectra and bins as well as identical x-values. In particular, this algorithm will be used for data reduction workflow algorithms.

Bug Fixes
---------

- Bin masking information was wrongly saved when saving workspaces into nexus files, which is now fixed.

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
