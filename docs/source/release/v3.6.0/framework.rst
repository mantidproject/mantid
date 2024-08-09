=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Load Instrument Breaking Change
-------------------------------

-  :ref:`Load <algm-Load>` instrument is an algorithm used by many of the Loading
   algorithms in Mantid behind the scenes, for those this has been
   adjusted, but if you call :ref:`LoadInstrument <algm-LoadInstrument>` in your own scripts ...
-  The RewriteSpectraMap property of :ref:`LoadInstrument <algm-LoadInstrument>` must be specified as
   of version 3.6. This has previously been optional.

Algorithms
----------

The usage of algoirthms is now tracked so we can get a better idea of
which algorithms are used, and which are not. No personal information is
stored alongside this, just a count of algorithm usages against the
version of Mantid.

New
###

-  :ref:`Comment <algm-Comment>`: A simple algorithm to add comments into the history of a workspace
-  :ref:`ExportGeometry <algm-ExportGeometry>`: Writes portions of the instrument geometry in IDF xml format.
-  :ref:`StringToPng <algm-StringToPng>` Writes a simple text message into a png file. This can be used in
   autoreduction, if users expect an image output.
-  :ref:`ExtractUnmaskedSpectra <algm-ExtractUnmaskedSpectra>`: Extracts unmasked spectra from a workspace and places them in a new
   workspace.
-  :ref:`AccumulateMD <algm-AccumulateMD>`: Adds data to an existing MDEventWorkspace from a list of input files.
   This allows accumulation of data in a workspace during an experiment.

Improved
########

-  :ref:`ExportTimeSeriesLog <algm-ExportTimeSeriesLog>`
   has been improved such that it is able to export time series log with
   various time unit, to export the time as relative time or epoch time,
   and export partial time series log.
-  :ref:`LoadEventNexus <algm-LoadEventNexus>`
   previously had a bug that could cause the number of events to be
   doubled when loading a subset of spectra. This has been tracked down
   and resolved, along with improving the performance for loading a
   subset of spectra.
-  :ref:`LoadFITS <algm-LoadFITS>`:
   has been optimized. Support for additional header keys has been
   added. Several issues have been fixed.
-  :ref:`LoadIsawPeaks <algm-LoadIsawPeaks>`
   has been optimized for loading of large files. A test file with 6000
   peaks that previously took 2.5 minutes to load now takes 7 seconds.
-  :ref:`LoadLogPropertyTable <algm-LoadLogPropertyTable>`
   can now handle event files, and filenames with characters after the
   run number, such as \`CNCS\_7860\_event.nxs\`.
-  :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>`
   has been fixed so that it loads spectrum numbers from files where the
   up axis is a numeric axis. As a result :ref:`CompareWorkspaces <algm-CompareWorkspaces>` now passes
   with a workspace in memory and one that is passed through save/load.
-  :ref:`NormaliseByCurrent <algm-NormaliseByCurrent>`
   gives more specific and detailed error messages, especially when
   normalizing multiperiod data.
-  :ref:`RemoveBins <algm-RemoveBins>`
   has been fixed so it now properly processes workspaces index requests
   > 0.
-  :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>`
   has been fixed so files do not complain about "Error parsing duration
   in algorithm history entry." on reloading.
-  :ref:`UpdateInstrumentFromFile <algm-UpdateInstrumentFromFile>`
   had a bug where the algorithm would crash if it encountered a
   spectrum that had multiple detectors assigned to it. This has been
   fixed.

Deprecated
##########

-  AddNote v1 is deprecated in favour of :ref:`Comment <algm-Comment>` v1.
-  :ref:`LoadAscii <algm-LoadAscii>` v1 is deprecated in favour of v2.
-  :ref:`LoadEventPreNexus <algm-LoadEventPreNexus>` v1 is deprecated in favour of v2.

MD Algorithms (VATES CLI)
#########################

Many of these areas will see further improvements in the following
release:

-  Added progress reporting and interruptibility to several MD
   algorithms.
-  Fixed several bugs related to dimension naming in MDWorkspaces.
-  Added the
   :ref:`AccumulateMD <algm-AccumulateMD>` algorithm.
-  :ref:`TransposeMD <algm-TransposeMD>` can now be used as :ref:`TransposeMD <algm-TransposeMD>`
   which will be more familiar for users of Matlab and Horace.
-  Data in MDWorkspaces which is masked using :ref:`MaskMD <algm-MaskMD>` will no longer be
   displayed in 1D plots, the Slice Viewer or the Vates Simple
   Interface.
-  Data in MDWorkspaces which is masked using :ref:`MaskMD <algm-MaskMD>` is now set to *not
   a number* (NaN).

Performance
-----------

-  The performance of loading a partial subset of spectra from Event
   Nexus files has been greatly increased.
-  The performance of loading instruments with lots of rectangular
   detector banks has been notably increased.
-  Loading of ISAW Peaks files has been optimized. A test file with 6000
   peaks that previously took 2.5 minutes to load now takes 7 seconds.
   Another file with 157,377 lines that reportedly took about one hour
   to load now loads in roughly 8 seconds.
-  Removed unnecessary clone of surfaces in Mantid::Geometry::Rule.


CurveFitting
------------

Improved
########

-  :ref:`StretchedExpFT <func-StretchedExpFT>` includes now fitting parameter "origin". It enables
   shifting the peak center.

Python
------

-  `#14022 <https://github.com/mantidproject/mantid/issues/14022>`_
   Monitor workspace getters and setters have been exposed to Python.
   This allows users to substantially simplify the reduction workflow,
   in particular for direct inelastic experiments in ISIS.
   :ref:`RenameWorkspace <algm-RenameWorkspace>`
   algorithm supports monitors workspace, attached to a workspace and
   renames both workspaces accordingly (Monitor workspace is named the
   same as main workspace with **\_monitors** suffix at the end. )
-  TableWorkspace.addRow() now accepts anything that behaves like a
   sequence (tuples, numpy arrays, etc) rather than just lists and
   dictionaries.
-  `#13751 <https://github.com/mantidproject/mantid/issues/13751>`_
   Plot normalization can now be controlled from the python
   plotSpectrum() function using the distribution keyword argument.

Python Algorithms
#################

-  Python Algorithms should now define which category the algorithm
   should be shown under in the Algorithms toolbox. For example this
   definition specifies that this algorithm should be listed twice, once
   under Arithmetic, and again under
   CorrectionFunctions\\\\SpecialFunctions. More details can be found in
   `Basic PythonAlgorithm Structure <Basic PythonAlgorithm Structure>`_.

.. code:: python

    class HelloWorld(PythonAlgorithm):

        def category(self):
            return 'Arithmetic;CorrectionFunctions\\SpecialFunctions'

-  If you do not define this method the algorithm will still register in
   MantidPlot, and work, but Mantidplot will warn you that the algorithm
   does not have a category defined, and it will appear under the
   PythonAlgorithms category in the Algorithms Toolbox.

IPython Notebooks
#################

-  IPython notebooks generated using the :ref:`GenerateIPythonNotebook <algm-GenerateIPythonNotebook>`
   algorithm, or from the ISIS Reflectometry interface, are now looking
   more friendly. This is thanks to importing more functionality from
   Mantid rather than the code being in the notebook.


Full list of
`Framework <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.6%22+is%3Amerged+label%3A%22Component%3A+Framework%22>`_
and
`Python <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.6%22+is%3Amerged+label%3A%22Component%3A+Python%22>`_
changes on GitHub

