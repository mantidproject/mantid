===================
Load Algorithm Hook
===================

.. contents::
  :local:

The Load Algorithm
##################
This page describes how to register a new file loading algorithm so that it can be used through
the general-purpose :ref:`Load <algm-Load>` algorithm.
The :ref:`Load <algm-Load>` algorithm is a special algorithm that does very little loading itself.
It instead tries to search for the most suitable algorithm for a given file and then uses this
algorithm as a child to load the file. An algorithm wishing to be included as part of the search
must register itself slightly differently and not use ``DECLARE_ALGORITHM`` macro.

The process of searching for the correct loader needs to be fairly quick as it will be especially
noticeable in the GUI if the process takes a long time. To speed up the process the loaders are
currently split into 2 categories:

- Nexus (HDF5)
- Legacy Nexus (HDF4/5 -- for ISIS muon scattering only)
- Non-HDF

A quick check is performed, using ``H5::H5File::isHdf5()``, to test whether the file looks like a
`HDF5 <http://www.hdfgroup.org/>`__ file.
If the check succeeds then only the HDF5 group of loaders are checked.
Another quick check is performed to test whether the file has an HDF4 signature.  If this succeeds, then only the Legacy Nexus loaders are checked.
If the file is neither HDF5 nor HDF4, then the non-HDF group of loaders are checked.

Descriptors
###########
To avoid the file being opened/closed by each ``confidence`` method, a ``Descriptor`` object is provided.
The actual ``Descriptor`` class depends on whether the file is a HDF file or not.

Nexus
-----
To register an algorithm as a Nexus loader use the IFileLoader<NexusDescriptorLazy> interface as a base class for your algorithm.
In your cpp file include the ``MantidAPI/RegisterFileLoader.h`` header and use the ``DECLARE_NEXUS_FILELOADER_ALGORITHM``
macro *instead* of the standard ``DECLARE_ALGORITHM`` macro.

The interface requires that the method ``virtual int confidence(Nexus::NexusDescriptorLazy &descriptor) const = 0;``
be overloaded in the inheriting class. It should use the provided descriptor to check whether the loader is
able to load the wrapped file and return a confidence level as an integer.

NexusDescriptorLazy
^^^^^^^^^^^^^^^^^^^
The lazy Nexus descriptor perform a shallow preload of the file, and then only lazily checks for entries (without opening them)
as loaders query them.  It owns a handle to an HDF5 file.  This is also able to load string data, if necessary for determining file loader.
Avoid calling the ``getAllEntries()`` method; doing so may lead to unexpected or inefficient results.

Do **NOT** open a ``Nexus::File`` object using the descriptor's filename; doing so defeats efficiency gains of using a shallow loader.

All new algorithms for loading HDF5 data should use the lazy descriptor.

NexusDescriptor
^^^^^^^^^^^^^^^
This Nexus descriptor should not be used for confidence checks in `Load`, as it performs an eager load of the entire filetree.
Use the lazy descriptor instead.

LegacyNexus
-----------
These loaders preserve the state of ``Mantid::Nexus`` prior to the Nexus refactor.  They still rely on ``napi`` for loading HDF4 files,
and do not include many of the performance efficiency gains from the refactor.  This type of loader should only be used for algorithms
meant to load ISIS muon files, many of which still use HDF4.

No new loaders should be written to use the legacy Nexus subpackage.

LegacyNexus
-----------
These loading algorithms are based on the old ``napi`` framework, and do not include improvements in efficiency and memory use made
during the Nexus refactor.  They remain in this state only for use by muon scattering scientists at ISIS.  These are the only algorithms
in Mantid capable of loading HDF4 files.  No new algorithms should be registered in this state.

LegacyNexusDescriptor
^^^^^^^^^^^^^^^^^^^^^
This uses old ``napi`` behavior to load the entire filetree and store in a dictionary cache.  Parts of the tree may be queried using available methods.

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
