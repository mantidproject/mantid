.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given a PeaksWorkspace with a UB matrix corresponding to a Niggli
reduced cell, this algorithm will display a list of possible
conventional cells. The max scalar error property sets a limit on the
maximum allowed error in the cell scalars, to restrict the list to
possible cells that are a good match for the current reduced cell. The
list can also be forced to contain only the best fitting conventional
cell for each Bravais lattice type, by setting the best only property to
true.

This algorithm is based on the paper: "Lattice Symmetry and
Identification -- The Fundamental Role of Reduced Cells in Materials
Characterization", Alan D. Mighell, Vol. 106, Number 6, Nov-Dec 2001,
Journal of Research of the National Institute of Standards and
Technology, available from:
nvlpubs.nist.gov/nistpubs/jres/106/6/j66mig.pdf.

Usage
-----

**Example:**

.. testcode:: ExShowPossibleCells

   ws=LoadIsawPeaks("TOPAZ_3007.peaks")
   FindUBUsingFFT(ws,MinD=8.0,MaxD=13.0)
   ShowPossibleCells(PeaksWorkspace=ws)
   alg = ws.getHistory().lastAlgorithm()
   print "Num Cells : ", alg.getPropertyValue("NumberOfCells")


Output:

.. testoutput:: ExShowPossibleCells

   Num Cells :  2


.. categories::
