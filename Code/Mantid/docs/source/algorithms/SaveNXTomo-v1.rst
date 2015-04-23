.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Save one or more workspaces (of type :ref:`Workspace2D <Workspace2D>`)
into a NeXus NXtomo file (NeXus application definition format). This
algorithm has been designed to save workspaces such as those
containing data from FITS images that the algorithm
:ref:`algm-LoadFITS` produces. Different workspaces may not be saved
properly (it may not make sense to save them into NXTomo files) or may
produce errors in this algorithm. When the input workspace given is a
:ref:`Workspace2D <Workspace2D>`, that single workspace is written to
the output NXTomo file. When the input workspace is a
:ref:`WorkspaceGropu <WorkspaceGroup>` (as LoadFITS produces), all the
:ref:`Workspace2D <Workspace2D>` workspaces included in the group will
be written to the output file.

Depending on the value given to the property *OverwriteFile* it will
create a new NXTomo file and add into it the data from the input
workspace, or it will append the data such that a sequence of calls to
this algorithm can cumulatively add new image files into a same output
NXTomo file. Possible uses of this algorithm include writing NXTomo
files with stacks of images to be used as inputs to tomographic
reconstruction tools.

.. note: not including a doc test because this requires loading and
   saving files. This class is decently tested for now in its unit
   test. A fake-workspace based example would be confusing to users,
   unless we add a 'CreateFITSLikeWorkspace' helper which seems an
   overkill.

.. code-block:: python

   # Usually you'll save image data loaded from FITS files
   LoadFITS('example1.fits', OutputWorkspace='reconstruction'))
   LoadFITS('example2.fits', OutputWorkspace='reconstruction'))
   # Write NXTomo file with data from 2 images
   SaveNXTomo(InputWorkspaces='reconstruction', Filename='input_rec')

**NOTE:** this algorithm is currently subject to changes and
extensions, as new functionality for imaging and tomographic
reconstruction is integrated in Mantid. At the moment it uses the
NXTomo file format specified in the `NeXus NXTomo application
definition
<http://download.nexusformat.org/sphinx/classes/applications/NXtomo.html>`__,
but extensions and/or different variants might be added in the future.

.. categories::
