.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm allows instrument parameters to be saved into an
`instrument parameter file <http://mantidproject.org/InstrumentParameterFile>`__.
The parameter file can then be inspected and or modified. It can also be loaded back into
Mantid using the `LoadParameterFile <http://mantidproject.org/LoadParameterFile>`__ algorithm.

The LocationParameters property specifies whether or not to save any calibration parameters
used to adjust the location of any components. Specifically, it will skip "x", "y", "z",
"r-position", "t-position", "p-position", "rotx", "roty", and "rotz" parameters.

Usage
-----

**Example - save an instrument's parameters to a file:**

.. testcode:: ExParametersSimple

  Load(Filename="MAR11001.raw", OutputWorkspace="MAR11001", LoadMonitors="Separate")
  SaveParameterFile(Workspace="MAR11001", Filename="/tmp/params.xml", LocationParameters=False)

  pfile = open("/tmp/params.xml", "r")
  lines = pfile.readlines()
  pfile.close()

  for line in lines[0:6]:
    print(line.strip())
  print("etc...")

.. testcleanup:: ExParametersSimple

   import os
   os.remove("/tmp/params.xml")

Output:

.. testoutput:: ExParametersSimple

  <?xml version="1.0" encoding="UTF-8"?>
  <parameter-file instrument="MARI" valid-from="1900-01-31T23:59:59">
  <component-link name="MARI">
  <parameter name="DelayTime">
  <value val="-3.9"/>
  </parameter>
  etc...

.. categories::
