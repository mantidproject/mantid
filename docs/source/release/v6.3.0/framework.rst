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
- Introducing a naming convention for algorithms, and *deprecated aliases* as the preferred method for renaming a C++ or Python algorithm.
- Enabling deprecation of Python algorithms; instructions on how to deprecate a C++ or Python algorithm in the developer documentation.

Improvements
############

- :ref:`GenerateLogbook <algm-GenerateLogbook>` now allows to perform binary operations even when certain entries do not exist, e.g. to create a string with all polarisation orientations contained in a collection of data files
- Event nexuses produced at ILL can now be loaded using :ref:`LoadEventNexus <algm-LoadEventNexus>`.
- :ref:`Rebin <algm-Rebin>` now has an option for binning with reverse logarithmic and inverse power bins.
- :ref:`SetSample <algm-SetSample>` can now load sample environment XML files from any directory using ``SetSample(ws, Environment={'Name': 'NameOfXMLFile', 'Path':'/path/to/file/'})``.

Data Objects
------------

Python
------

Installation
------------

MantidWorkbench
---------------

See :doc:`mantidworkbench`.

SliceViewer
-----------

Improvements
############

- :ref:`SaveAscii <algm-SaveAscii>` has new property OneSpectrumPerFile.

Bugfixes
########

- Fixed bug in :ref:`Run <Run>` goniometer when using :ref:`algm-Plus`.

:ref:`Release 6.3.0 <v6.3.0>`
