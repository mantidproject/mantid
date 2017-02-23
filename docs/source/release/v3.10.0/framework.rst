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

- Two new properties were added to :ref:`algm-Integration`: *RangeLowerList* and *RangeUpperList* can be used to give histogram-specific integration ranges.

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

- For multiple output parameters, python algorithms now return a `namedtuple` instead of a tuple. Old scripts should still work,
  but one can now do

  .. code-block:: python

      results = GetEi(w)
      print(results)
      print(results.IncidentEnergy)
      print(results[0])

  This will yield:

  .. code-block:: python

      GetEi_returns(IncidentEnergy=3.0, FirstMonitorPeak=0.0, FirstMonitorIndex=0, Tzero=61.77080180287334)
      3.0
      3.0



Python Algorithms
#################

|

Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`__
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Python%22>`__
changes on GitHub
