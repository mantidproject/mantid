.. _Directtools Python module:

==================
:mod:`directtools`
==================

:literal:`directtools` is a Python module for quickly plotting standardized :math:`S(Q,E)` color fill plots as well as line profiles (cuts) in constant :math:`Q` and :math:`E`. The module also provides a few utility functions for inspecting and manipulating the :math:`S(Q,E)` workspace.

For a general introduction on using :mod:`matplotlib` with Mantid, see :ref:`this introduction <plotting>`

.. plot::
   :include-source:

   import directtools as dt
   from mantid.simpleapi import *

   DirectILLCollectData(Run='ILL/IN4/084447', OutputWorkspace='data')
   DirectILLReduction(InputWorkspace='data', OutputWorkspace='SofQW')

   fig, ax = dt.plotSofQW('SofQW')
   #fig.show()

Reference
=========

Classes
#######

.. autoclass:: directtools.SampleLogs
   :members: __init__

Functions
#########

.. automodule:: directtools
   :members: box2D, defaultrcParams, dynamicsusceptibility, nanminmax, plotconstE,
             plotconstQ, plotcuts, plotprofiles, plotSofQW, subplots, validQ, wsreport

.. categories:: Techniques
