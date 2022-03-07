=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------

- :ref:`LoadAndMerge <algm-LoadAndMerge>` will now offer a possibility to have a joined workspace as output instead of a workspace group.
- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` will now have a possibility to set a linear integer range as the axis of the output joined workspace.
- :ref:`CalculateFlux <algm-CalculateFlux>` will now work also on workspaces with dimensionless x-axis.
- Performance of :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` improved when running with ImportanceSampling=True

Data Objects
------------

Python
------

- Added possibility to forward log messages to Python, see :ref:`mantid.utils.logging.log_to_python`.

Installation
------------

MantidWorkbench
---------------

See :doc:`mantidworkbench`.

SliceViewer
-----------

Improvements
############

Bugfixes
########

:ref:`Release 6.4.0 <v6.4.0>`
