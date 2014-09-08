.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given a PeaksWorkspace with a UB matrix corresponding to a Niggli
reduced cell, this algorithm will allow the user to select a
conventional cell with a specified cell type and centering. If the apply
flag is not set, the information about the selected cell will just be
displayed. If the apply flag is set, the UB matrix associated with the
sample in the PeaksWorkspace will be updated to a UB corresponding to
the selected cell AND the peaks will be re-indexed using the new UB
matrix. NOTE: The possible conventional cells, together with the
corresponding errors in the cell scalars can be seen by running the
ShowPossibleCells algorithm, provided the stored UB matrix corresponds
to a Niggli reduced cell.

This algorithm is based on the paper: "Lattice Symmetry and
Identification -- The Fundamental Role of Reduced Cells in Materials
Characterization", Alan D. Mighell, Vol. 106, Number 6, Nov-Dec 2001,
Journal of Research of the National Institute of Standards and
Technology, available from: `nvlpubs <nvlpubs.nist.gov/nistpubs/jres/106/6/j66mig.pdf>`_.

Usage
-----

**Example:**

.. testcode:: ExSelectCellOfType

   ws=LoadIsawPeaks("TOPAZ_3007.peaks")
   FindUBUsingFFT(ws,MinD=8.0,MaxD=13.0)
   print "Lattice before SelectCellOfType:"
   lattice = ws.sample().getOrientedLattice()
   print lattice.a(),lattice.b(),lattice.c(),lattice.alpha(),lattice.beta(),lattice.gamma()
   SelectCellOfType(PeaksWorkspace=ws, CellType='Monoclinic', Centering='C', Apply=True)
   print "\nLattice after SelectCellOfType:"
   lattice = ws.sample().getOrientedLattice()
   print lattice.a(),lattice.b(),lattice.c(),lattice.alpha(),lattice.beta(),lattice.gamma()


Output:

.. testoutput:: ExSelectCellOfType

   Lattice before SelectCellOfType:
   8.60581864273 11.935925461 11.9418127661 107.429088323 98.7529124665 98.9511934747

   Lattice after SelectCellOfType:
   14.1310511523 19.247332564 8.60581864273 89.8811706749 105.07133377 89.970386662

.. categories::
