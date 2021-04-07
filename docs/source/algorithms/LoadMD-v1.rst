.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads a :ref:`MDEventWorkspace <MDWorkspace>` that was
previously saved using the :ref:`algm-SaveMD` algorithm to a .nxs file
format.

If the workspace is too large to fit into memory, You can load the workspace
as a :ref:`file-backed MDWorkspace <MDWorkspace File Backed>` by checking the
FileBackEnd option. This will load the box structure (allowing for some
visualization with no speed penalty) but leave the events on disk until
requested. Processing file-backed MDWorkspaces is significantly slower than
in-memory workspaces due to frequent file access!

For file-backed workspaces, the Memory option allows you to specify a
cache size, in MB, to keep events in memory before caching to disk.

Finally, the BoxStructureOnly and MetadataOnly options are for special
situations and used by other algorithms, they should not be needed in
daily use.

Usage
-----
.. include:: ../usagedata-note.txt

**Example - Load MD workspace.**

.. testcode:: ExLoadMD

   # Load sample MDEvent workspace, present in Mantid unit tests
   mdws = LoadMD('MAPS_MDEW.nxs');

   # Check results
   print("Workspace type is: {}".format(mdws.id()))
   print("Workspace has:{0:2} dimensions and contains: {1:4} MD events".format(mdws.getNumDims(),mdws.getNEvents()))

Output:

.. testoutput:: ExLoadMD

   Workspace type is: MDEventWorkspace<MDLeanEvent,4>
   Workspace has: 4 dimensions and contains:    0 MD events

.. categories::

.. sourcelink::
