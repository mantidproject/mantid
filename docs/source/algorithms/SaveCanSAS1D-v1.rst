.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

.. _Version 1.0: http://www.cansas.org/formats/1.0/cansas1d.xsd
.. _version 1.1: http://www.cansas.org/formats/1.1/cansas1d.xsd
.. _canSAS Wiki: http://www.cansas.org/formats/canSAS1d/1.1/doc/

Description
-----------

Saves the given :ref:`MatrixWorkspace` to a file in the canSAS 1-D format.

The canSAS 1-D Format
#####################

The canSAS 1-D standard for reduced 1-D SAS data is implemented using XML
files. A single file can contain SAS data from a single experiment or multiple
experiments.

`Version 1.0`_ of the canSAS 1-D schema can be used to validate files of this format.

There is a `canSAS Wiki`_ available which provides more information about the format.

Versions
########

This is version 1 of the algorithm, which meets version 1.0 of the canSAS 1-D specification.

You can export data to files using `version 1.1`_ of the specification by using :ref:`version 2 of SaveCanSAS1D <algm-SaveCanSAS1D-v2>`.

Usage
-----

**Example - Save/Load "Roundtrip"**

.. testcode:: ExSimpleSavingRoundtrip

   import os

   # Create dummy workspace.
   dataX = [0,1,2,3]
   dataY = [9,5,7]
   out_ws = CreateWorkspace(dataX, dataY, UnitX="MomentumTransfer")

   file_path = os.path.join(config["defaultsave.directory"], "canSASData.xml")

   # Do a "roundtrip" of the data.
   SaveCanSAS1D(out_ws, file_path, Version=1)
   in_ws = LoadCanSAS1D(file_path)

   print("Contents of the file = " + str(in_ws.readY(0)) + ".")

.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the file = [9. 5. 7.].

.. categories::

.. sourcelink::
