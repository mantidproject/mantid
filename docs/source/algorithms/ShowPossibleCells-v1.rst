.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a PeaksWorkspace with a :ref:`UB matrix <Lattice>` corresponding
to a Niggli reduced cell, this algorithm will display a list of possible
conventional cells. The max scalar error property sets a limit on the
maximum allowed error in the cell scalars, to restrict the list to
possible cells that are a good match for the current reduced cell. The
list can also be forced to contain only the best fitting conventional
cell for each Bravais lattice type, by setting the best only property to
true.

This algorithm is based on the paper: Alan D. Mighell, *Lattice
Symmetry and Identification—The Fundamental Role of Reduced Cells in
Materials Characterization.* Journal of research of the National
Institute of Standards and Technology **106.6** (2001): 983, available
from: `nvlpubs <http://nvlpubs.nist.gov/nistpubs/jres/106/6/j66mig.pdf>`_.

Usage
-----

**Example:**

.. testcode:: ExShowPossibleCells

   import json
   ws = LoadIsawPeaks("TOPAZ_3007.peaks")
   FindUBUsingFFT(ws,MinD=8.0,MaxD=13.0)
   number_of_cells, cells = ShowPossibleCells(PeaksWorkspace=ws)
   print(f"Num Cells: {number_of_cells}")
   cell1 = json.loads(cells[0])
   cell2 = json.loads(cells[1])
   print(f"First cell: Error={cell1['Error']:.3f} FormNumber={cell1['FormNumber']}, CellType={cell1['CellType']}, Centering={cell1['Centering']}")
   print(f"Second cell: Error={cell2['Error']:.3f} FormNumber={cell2['FormNumber']}, CellType={cell2['CellType']}, Centering={cell2['Centering']}")

   # the output can then be used by SelectCellOfType
   SelectCellOfType(ws, CellType=cell1['CellType'], Centering=cell1['Centering'], Apply=True)
   # or SelectCellWithForm
   SelectCellWithForm(ws, FormNumber=cell1['FormNumber'], Apply=True)
   # or SetUB
   SetUB(ws, UB=cell1["UB"])


Output:

.. testoutput:: ExShowPossibleCells

   Num Cells: 2
   First cell: Error=0.022 FormNumber=25, CellType=Monoclinic, Centering=C
   Second cell: Error=0.000 FormNumber=44, CellType=Triclinic, Centering=P


.. categories::

.. sourcelink::
