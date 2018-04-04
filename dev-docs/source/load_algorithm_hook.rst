===================
Load Algorithm Hook
===================

.. contents::
  :local:

The Load Algorithm
##################
This page describes how to register a new file loading algorithm so that it can be used through
the general-purpose :ref:`Load <algm-Load>` algorithm.
The :ref:`Load <algm-Load>` algorithm is a special algorithm as does very little work itself.
It instead tries to search for the most suitable algorithm for a given file and then uses this
algorithm as a child to load the file. An algorithm wishing to be included as part of the search
must register itself slightly differently and not use ``DECLARE_ALGORITHM`` macro.

The process of searching for the correct loader needs to be fairly quick as it will be especially
noticeable in the GUI if the process takes a long time. To speed up the process the loaders are
currently split into 2 categories:

- HDF - Currently only HDF4/5
- NonHDF - The rest

A quick check is performed, using ``HDFDescriptor::isHDF()``, to test whether the file looks like a
`HDF <http://www.hdfgroup.org/>`__ file.
If the check succeeds then only the HDF group of loaders are checked, otherwise only the NonHDF group are checked.

Descriptors
###########
To avoid the file being opened/closed by each ``confidence`` method, a ``Descriptor`` object is provided.
The actual ``Descriptor`` class depends on whether the file is a HDF file or not.

HDF
---
To register an algorithm as a HDF loader use the IHDFLoader interface as a base class for your algorithm.
In your cpp file include the ``MantidAPI/RegisterFileLoader.h`` header and use the ``DECLARE_HDF_FILELOADER``
macro *instead* of the standard ``DECLARE_ALGORITHM`` macro.

The interface requires that the method ``virtual int confidence(Kernel::HDFDescriptor & descriptor) const = 0;``
be overloaded in the inheriting class. It should use the provided descriptor to check whether the loader is
able to load the wrapped file and return a confidence level as an integer.

HDFDescriptor
^^^^^^^^^^^^^
This provides methods to query characteristics of the current file to avoid repeated accessing of the tree.

Non-HDF
-------
To register an algorithm as a Non-HDF loader use the ``IFileLoader`` interface as a base class for your algorithm.
In your cpp file include the ``MantidAPI/RegisterFileLoader.h`` header and use the ``DECLARE_FILELOADER`` macro
*instead* of the standard ``DECLARE_ALGORITHM`` macro.

The interface requires that the method ``virtual int confidence(Kernel::FileDescriptor & descriptor) const = 0;``
be overloaded in the inheriting class. It should use the provided descriptor to check whether the loader is
able to load the wrapped file and return a confidence level as an integer.

FileDescriptor
^^^^^^^^^^^^^^

This provides methods to query some characteristics of the current file and also access the open stream to avoid
repeated opening/closing of the file. *Avoid* closing the stream. The code calling the ``confidence`` method ensures
that the stream is back at the start of the file before checking the next loader so simply use the stream as
necessary and leave it as it is.
