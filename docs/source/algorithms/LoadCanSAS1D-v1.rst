.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

.. _version 1.0: http://www.cansas.org/formats/1.0/cansas1d.xsd
.. _Version 1.1: http://www.cansas.org/formats/1.1/cansas1d.xsd
.. _canSAS Wiki: http://www.cansas.org/formats/canSAS1d/1.1/doc/

Description
-----------

Loads the given file, which should be in the canSAS1d XML format specified
by canSAS 1D Data Formats Working Group schema `Version 1.0`_ and
creates output workspace. There is a `canSAS Wiki`_ available which provides
more information about the format.

If the file contains multiple SASentry elements a workspace group will
be created and each SASentry will be one workspace in the group. Loading
multiple SASdata elements is not supported.

Versions
########

This is version 1 of the algorithm, which meets version 1.0 of the canSAS1d specification.

You can load files using `version 1.1`_ of the specification by using :ref:`version 2 of LoadCanSAS1D <algm-LoadCanSAS1D-v2>`.

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
   in_ws = LoadCanSAS1D(file_path, Version=1)

   print("Contents of the file = {}.".format(in_ws.readY(0)))

.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the file = [9. 5. 7.].

.. categories::

.. sourcelink::
