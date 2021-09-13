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

Improvements
############

- :ref:`SaveAscii <algm-SaveAscii>` and :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>` have a new property OneSpectrumPerFile, controlling whether or not to save each spectrum in an individual file or all the spectra into a single file.
- :ref:`GenerateLogbook <algm-GenerateLogbook>` now allows to perform binary operations even when certain entries do not exist, e.g. to create a string with all polarisation orientations contained in a collection of data files.
- Event nexuses produced at ILL can now be loaded using :ref:`LoadEventNexus <algm-LoadEventNexus>`.

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

Bugfixes
########
- Fix out-of-range error when trying to access the projection matrix in sliceviewer for a workspace with a non-Q axis before other Q axes.
- For MDHisto workspaces get the projection matrix from the basis vectors on the workspace rather than search for the W_MATRIX log.


:ref:`Release 6.3.0 <v6.3.0>`
