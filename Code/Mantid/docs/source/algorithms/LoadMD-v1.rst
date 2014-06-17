.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm loads a `MDEventWorkspace <MDEventWorkspace>`__ that was
previously saved using the :ref:`algm-SaveMD` algorithm to a .nxs file
format.

If the workspace is too large to fit into memory, You can load the
workspace as a `file-backed
MDWorkspace <MDWorkspace#File-Backed_MDWorkspaces>`__ by checking the
FileBackEnd option. This will load the box structure (allowing for some
visualization with no speed penalty) but leave the events on disk until
requested. Processing file-backed MDWorkspaces is significantly slower
than in-memory workspaces due to frequency file access!

For file-backed workspaces, the Memory option allows you to specify a
cache size, in MB, to keep events in memory before caching to disk.

Finally, the BoxStructureOnly and MetadataOnly options are for special
situations and used by other algorithms, they should not be needed in
daily use.

.. categories::
