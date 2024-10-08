
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves the Instrument geometry from the workspace to a Nexus file. Saves detector positions and rotations.
Current version (1.0) does NOT save instrument shapes of instrument components. file is compliant to Nexus standards.
For more information on the Nexus format, see https://www.nexusformat.org/

the Instrument will be extracted from the specified workspace, and written to the specified location.

The (optional) H5 root group name is the parent NXentry group in which the Instrument and sample data are stored.
If no name is given, the root group will have a default name of 'entry'


Usage
-----

**Example - basic example using SaveNexusGeometry, with default H5 root group**

.. testcode:: SaveNexusGeometryExampleDefault

	import os

	ws = CreateSampleWorkspace()
	file_name  = "example_save_nexus_geometry.nxs"
	path = os.path.join(os.path.expanduser("~"), file_name)

	SaveNexusGeometry(ws, path)
	print(os.path.isfile(path))



Output:

.. testoutput:: SaveNexusGeometryExampleDefault

	True

.. testcleanup:: SaveNexusGeometryExampleDefault

    import os
    def removeFiles(files):
      for ws in files:
        try:
          path = os.path.join(os.path.expanduser("~"), ws)
          os.remove(path)
        except:
          pass

    removeFiles([file_name])

.. categories::

.. sourcelink::
