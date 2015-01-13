.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads the given file in the RKH text format, which can be a file with
three columns of numbers. If the FirstColumnValue is a recognised
:ref:`Mantid unit <Unit Factory>` the workspace is created with just one
spectrum. Alteratively if FirstColumnValue is set to 'SpectrumNumber'
then the workspace can have many spectra with the spectrum ID's equal to
the first column in the file.

Usage
-----

**Example - Save/Load "Roundtrip"**

.. testcode:: ExSimpleSavingRoundtrip

   import os
   import numpy

   # Create dummy workspace.
   dataX = [1,2,3,4,5]
   dataY = [8,4,9,7]
   dataE = [2,1,1,3]
   out_ws = CreateWorkspace(dataX, dataY, dataE, UnitX="MomentumTransfer")

   file_path = os.path.join(config["defaultsave.directory"], "example.out")

   # Do a "roundtrip" of the data.
   SaveRKH(out_ws, file_path)
   in_ws = LoadRKH(file_path)

   print "Contents of the file  = " + str(in_ws.readY(0))

.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the file  = [ 8.  4.  9.  7.]

.. categories::
