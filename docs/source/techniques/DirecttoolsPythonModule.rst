.. _Directtools Python module:

==================
:mod:`directtools`
==================

:mod:`directtools` is a Python module for quickly plotting standardized :math:`S(Q,E)` color fill plots as well as line profiles (cuts) in constant :math:`Q` and :math:`E`. The module also provides a few utility functions for inspecting and manipulating the :math:`S(Q,E)` workspace.

For a general introduction on using :mod:`matplotlib` with Mantid, see :ref:`this introduction <plotting>`

The input workspaces are expected to have some specific sample logs, namely ``instrument.name``, ``Ei``, ``run_number``, ``start_time``, ``sample.temperature``.

Examples
########

Examples of some key functionality of :mod:`directtools` is presented below. For a full reference of all available function and classes, see the Reference_ section below.

`plotSofQW`
~~~~~~~~~~~

The default parameters for :func:`directtools.plotSofQW` give a view of the :math:`S(Q,E)` workspace around the elastic peak with sensible limits for the axes and intensity:

.. plot::
   :include-source:

   import directtools as dt
   from mantid.simpleapi import *

   DirectILLCollectData(Run='ILL/IN4/084447.nxs', OutputWorkspace='data')
   DirectILLReduction(InputWorkspace='data', OutputWorkspace='SofQW')

   fig, ax = dt.plotSofQW('SofQW')
   #fig.show()

The :math:`Q`, :math:`E` and intensity limits can be changed using the optional parameters. The utility functions :func:`directtools.validQ` and :func:`directtools.nanminmax` might be helpful when determining suitable ranges:

.. plot::
   :include-source:

   import directtools as dt
   from mantid.simpleapi import *

   DirectILLCollectData(Run='ILL/IN4/084447.nxs', OutputWorkspace='data')
   DirectILLReduction(InputWorkspace='data', OutputWorkspace='SofQW')

   EMin = -20.
   QMax = dt.validQ('SofQW', EMin)[1]
   VMax = 0.5 * dt.nanminmax('SofQW')[1]

   fig, axes = dt.plotSofQW('SofQW', QMax=QMax, EMin=EMin, VMax=VMax)
   #fig.show()

Cuts in :math:`S(Q,E)`
~~~~~~~~~~~~~~~~~~~~~~

An important aspect of examining the :math:`S(Q,E)` workspace is to plot cuts at constant :math:`Q` and :math:`E`. This can be done by :func:`directtools.plotconstQ` and :func:`directtools.plotconstE`:

.. plot::
   :include-source:

   import directtools as dt
   from mantid.simpleapi import *
   import warnings
   
   DirectILLCollectData(Run='ILL/IN4/084447.nxs', OutputWorkspace='data')
   DirectILLReduction(InputWorkspace='data', OutputWorkspace='SofQW')
   
   Q = 2.
   dQ = 0.2
   # plotconstQ produces a warning on some versions of numpy.
   # The "with" statement catches this warning so that the automated
   # builds don't fail.
   with warnings.catch_warnings():
       warnings.simplefilter("ignore", category=UserWarning)
       fig, axes, cuts = dt.plotconstQ('SofQW', Q, dQ)
       #fig.show()

Any of the workspace, cut centre or cut width arguments can be a :class:`list` instead. This enables data comparison:

.. plot::
   :include-source:

   import directtools as dt
   from mantid.simpleapi import *
   
   DirectILLCollectData(Run='ILL/IN4/084447.nxs', OutputWorkspace='data')
   DirectILLReduction(InputWorkspace='data', OutputWorkspace='SofQW')
   
   Q1 = 2.
   Q2 = 3.
   dQ = 0.2
   fig, axes, cuts = dt.plotconstQ('SofQW', [Q1, Q2], dQ)
   #fig.show()

The :func:`directtools.plotconstQ` and :func:`directtools.plotconstE` functions use :func:`directtools.plotcuts` to do the actual line profiles and plotting. The profiles are made by the :ref:`algm-LineProfile` algorithm, and all three plotting functions return a list of the produced line profile workspace names.

If a line profile already exists, it can be plotted using :func:`directtools.plotprofiles`. This also accepts either a single line profile workspace or a list of workspaces enabling comparison:

.. plot::
   :include-source:

   import directtools as dt
   from mantid.simpleapi import *
   
   DirectILLCollectData(Run='ILL/IN4/084447.nxs', OutputWorkspace='data')
   DirectILLReduction(InputWorkspace='data', OutputWorkspace='SofQW')
   
   E1 = 8.
   dE = 2.
   cut1 = LineProfile('SofQW', E1, dE, 'Horizontal')
   label1 = 'At E = {} meV'.format(E1)
   E2 = E1 - 4.
   cut2 = LineProfile('SofQW', E2, dE, 'Horizontal')
   label2 = 'At E = {} meV'.format(E2)
   fig, axes = dt.plotprofiles([cut1, cut2], [label1, label2], style='m')
   axes.legend()
   #fig.show()

Plotting density of states
~~~~~~~~~~~~~~~~~~~~~~~~~~

The density of states data calculated by :ref:`ComputeIncoherentDOS <algm-ComputeIncoherentDOS>` can be plotted by the :func:`directtools.plotDOS` function. The function accepts a single workspace or a list of workspaces as its arguments. The example below shows a comparison of densities of states calculated from :math:`S(Q,E)` and :math:`S(2\theta,E)`:

.. plot::
   :include-source:

   import directtools as dt
   from mantid.simpleapi import *

   DirectILLCollectData(Run='ILL/IN4/084447.nxs', OutputWorkspace='data')
   DirectILLReduction(InputWorkspace='data', OutputWorkspace='SofQW', OutputSofThetaEnergyWorkspace='SofTW')
   dosFromStw = ComputeIncoherentDOS('SofTW')
   dosFromSqw = ComputeIncoherentDOS('SofQW')
   fig, axes = dt.plotDOS([dosFromStw, dosFromSqw], labels=[r'from $S(2\theta, E)$', r'from $S(Q,E)$'])
   #fig.show()

Convenience tools
~~~~~~~~~~~~~~~~~

:class:`directtools.SampleLogs` is a convenience class to import the sample logs of a workspace into a 'struct' like object in Python:

.. testcode:: SampleLogsEx

   import directtools as dt
   from mantid.simpleapi import *
   
   DirectILLCollectData(Run='ILL/IN4/084447.nxs', OutputWorkspace='data')
   DirectILLReduction(InputWorkspace='data', OutputWorkspace='SofQW')
   
   # Works on any workspace, not just S(Q,E).
   logs = dt.SampleLogs('SofQW')
   print(logs.instrument.name)
   print(logs.run_number)

.. testcleanup:: SampleLogsEx

   mtd.clear()

Output:

.. testoutput:: SampleLogsEx

   IN4
   84447

Reference
#########

Classes
~~~~~~~

.. autoclass:: directtools.SampleLogs
   :members: __init__

Functions
~~~~~~~~~

.. automodule:: directtools
   :members: box2D, defaultrcparams, dynamicsusceptibility, nanminmax, plotconstE,
             plotconstQ, plotcuts, plotprofiles, plotDOS, plotSofQW, subplots, validQ,
             wsreport

.. categories:: Techniques
