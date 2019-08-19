
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

The (optional) H5 root group name is the parent group in which the Instrument and sample data are stored.
If no name is given, the root group will have a default nme of 'entry'


Usage
-----

**Example - SaveNexusGeometry**

.. testcode:: SaveNexusGeometryExample

   # create sample workspace
ws1 = CreateSampleWorkspace()
instr1 = ws1.getInstrument()
compInfo = ws1.componentInfo()
detInfo = ws1.detectorInfo()

   # save Instrument in workspace with default H5 root name 'entry'
print("===========================")
#SaveNexusGeometry(workspace=ws1, FileName1="somePathToFile.nxs")
FileName1 = "path/to/someTestFile.nxs"
#SaveNexusGeometry(workspace=ws2, FileName="somePathToFile", H5Path="testRoot")
print("the Instrument {0} was saved to location {1}".format(instr1.getName(), FileName1))

  # save instrument in workspace with specified H5 root group name
print("===========================")
ws2 = CreateSampleWorkspace();
instr2 = ws2.getInstrument()
detInfo2 = ws2.detectorInfo()
compInfo2 = ws2.componentInfo()

FileName2 = "path/to/someTestFile.hdf5"
#SaveNexusGeometry(workspace=ws2, FileName="somePathToFile", H5Path="testRoot")
print("the Instrument {0} was saved to location {1}".format(instr2.getName(), FileName2))


Output:

.. testoutput:: SaveNexusGeometryExample

	CreateSampleWorkspace started
	CreateSampleWorkspace successful, Duration 0.00 seconds
	===========================
	the Instrument basic_rect was saved to location path/to/someTestFile.nxs
	===========================
	CreateSampleWorkspace started
	CreateSampleWorkspace successful, Duration 0.00 seconds
	the Instrument basic_rect was saved to location path/to/someTestFile.hdf5

.. categories::

.. sourcelink::

