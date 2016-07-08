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

- :ref:`FilterEvents <algm-FilterEvents>` now produces output
  workspaces with the same workspace numbers as specified by the
  ``SplittersWorkspace``.
- :ref:`ConvertAxisByFormula <algm-ConvertAxisByFormula>` now supports instrument geometry vairables and several constants within the formula.  Axes are now reversed if the need to be to maintain increasing axis values.

Deprecated
##########

MD Algorithms (VATES CLI)
#########################

Performance
-----------

CurveFitting
------------

- Added two new minimizers belonging to the trust region family of algorithms: DTRS and More-Sorensen.

Improved
########


Python
------


Python Algorithms
#################

Bug Fixes
---------
- Scripts generated from history including algorithms that added dynamic properties at run time (for example Fit, and Load) will not not include those dynamic properties in their script.  This means they will execute without warnings.


|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
