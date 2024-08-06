.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves the given workspace to a file which will be formatted in one of the historic ISIS SANS (‘COLETTE’/‘FISH’)
`data formats <https://www.isis.stfc.ac.uk/Pages/colette-ascii-file-format-descriptions.pdf>`__ devised by Richard K
Heenan.

1D or 2D workspaces may be saved.

If a 1D workspace is 'horizontal' (a single spectrum) then the first column in the three column output will contain the
X values of the spectrum (giving the bin centre if histogram data). For a 'vertical' (single column) 1D workspace,
the first column of the file will contain the spectrum number.

The created file can be reloaded using the :ref:`algm-LoadRKH` algorithm.

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

   print("Contents of the file  = " + str(in_ws.readY(0)))

.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the file  = [8. 4. 9. 7.]

.. categories::

.. sourcelink::
