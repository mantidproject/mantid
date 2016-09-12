.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm returns the nexus (HDF) files in the given files list that satisfy the specified criteria.
This is done without actually loading the data, but just the needed metadata.
Input files need to exist and be specified following the Mantid rules in `MultiFileLoading <http://www.mantidproject.org/MultiFileLoading>`_.
Note, that the ``+`` and ``-`` rules will act as ``,`` and ``:`` correspondingly, since the summing does not make sense for the given purpose.
Criteria could be any python logical expression involving the nexus entry names enclosed with ``$`` symbol.
Arbitrary number of criteria can be combined. The metadata entry should contain only one element.
Note, that if the entry is of string type, string comparison will be performed.
As a result, a plain comma separated list of fully resolved file names satisfying the criteria will be returned.
Note, that this algorithm requires `h5py <https://pypi.python.org/pypi/h5py>`_ package installed.

**Example - Running SelectNexusFilesByMetadata**

.. code-block:: ExSelectNexusFilesByMetadata

    res = SelectNexusFilesByMetadata(FileList='INTER00013460,13463,13464.nxs',
                                     NexusCriteria='$raw_data_1/duration$ > 1000 or $raw_data_1/good_frames$ > 10000')
    print("res is now a string containing comma separated paths of %i file names that satisfy the criteria" % len(res.split(',')))

Output:

.. code-block:: ExSelectNexusFilesByMetadata

   res is now a string containing comma separated paths of 2 file names that satisfy the criteria

.. categories::

.. sourcelink::