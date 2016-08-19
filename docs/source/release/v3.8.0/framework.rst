=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

- ``Facilities.xml`` was updated for changes to the SNS live data servers.

HistogramData
-------------

A new module for dealing with histogram data has been added, it is now being used internally in Mantid to store data in various workspace types.

- For C++ users, details can be found in the `transition documentation <http://docs.mantidproject.org/nightly/concepts/HistogramData.html>`_.
- For Python users, the interface is so far unchanged.
  However, to ensure data consistency and to reduce the risk of bugs, histograms now enforce length limitations. For example, there must be one bin edge more than data (Y and E) values.
  If you experience trouble, in particular exceptions about size mismatch, please refer to the section `Dealing with problems <http://docs.mantidproject.org/nightly/concepts/HistogramData.html#dealing-with-problems>`_.


Algorithms
----------

New
###

-  :ref:`ClearCache <algm-ClearCache>` an algorithm to simplify the clearance of several in memory or disk caches used in Mantid.

- :ref:`LoadPreNexusLive <algm-LoadPreNexusLive>` will load "live"
  data from file on legacy SNS DAS instruments.

- :ref:`CropToComponent <algm-CropToComponent>` allows for cropping a workspace to a list of component names.


Improved
########

- :ref:`FlatPlatePaalmanPingsCorrection <algm-FlatPlatePaalmanPingsCorrection>` & :ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`
  now accept 'Direct' as a possible ``EMode`` parameter.

- :ref:`FilterEvents <algm-FilterEvents>` now produces output
  workspaces with the same workspace numbers as specified by the
  ``SplittersWorkspace``.

- :ref:`SavePlot1D <algm-SavePlot1D>` has options for writing out
  plotly html files.

- :ref:`ConvertTableToMatrixWorkspace <algm-ConvertTableToMatrixWorkspace>`
  had a bug where the table columns were in a reversed order in the dialogue's combo boxes. 
  This is now fixed and the order is correct.

- :ref:`ConvertUnits <algm-ConvertUnits>` will no longer corrupt an in place workspace if the algorithm fails.

- :ref:`SetSample <algm-SetSample>`: Fixed a bug with interpreting the `Center` attribute for cylinders/annuli

- :ref:`ConvertToHistogram <algm-ConvertToHistogram>`: Performance improvement using new HistogramData module,
  3x to 4x speedup.

- :ref:`ConvertToPointData <algm-ConvertToPointData>`: Performance improvement using new HistogramData module,
  3x to 4x speedup.


Deprecated
##########

MD Algorithms (VATES CLI)
#########################

Performance
-----------

- The introduction of the HistogramData module may have influenced the performance of some algorithms and many workflows.
  A moderate number of algorithms should experience a speedup and reduced memory consumption.
  If you experience unusual slowdowns, please contact the developer team.

CurveFitting
------------

- Added two new minimizers belonging to the trust region family of algorithms: DTRS and More-Sorensen.

Improved
########


Python
------

- :py:obj:`mantid.kernel.MaterialBuilder` has been exposed to python
  and :py:obj:`mantid.kernel.Material` has been modified to expose the
  individual atoms.

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
