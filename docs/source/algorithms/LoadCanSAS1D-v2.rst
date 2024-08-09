.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads the given file, which should be in the CanSAS1d format specified
by canSAS 1D Data Formats Working Group schema
http://svn.smallangles.net/svn/canSAS/1dwg/trunk/cansas1d.xsd and
creates output workspace. CANSAS has a Wiki page at
http://www.smallangles.net/wgwiki/index.php/canSAS_Working_Groups

If the file contains multiple SASentry elements a workspace group will
be created and each SASentry will be one workspace in the group. Loading
multiple SASdata elements is not supported.

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
   SaveCanSAS1D(out_ws, file_path)
   in_ws = LoadCanSAS1D(file_path)

   print("Contents of the file = {}.".format(in_ws.readY(0)))

.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the file = [9. 5. 7.].

.. categories::

.. sourcelink::
