.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a set of peaks, and given a range of possible a,b,c values, this
algorithm will attempt to find a :ref:`UB matrix <Lattice>`, corresponding
to the Niggli reduced cell, that fits the data. The algorithm projects the
peaks on many possible direction vectors and calculates a Fast Fourier Transform
of the projections to identify regular patterns in the collection of
peaks. Based on the calcuated FFTs, a list of directions corresponding
to possible real space unit cell edge vectors is formed. The directions
and lengths of the vectors in this list are optimized (using a least
squares approach) to index the maximum number of peaks, after which the
list is sorted in order of increasing length and duplicate vectors are
removed from the list.

The algorithm then chooses three of the remaining vectors with the
shortest lengths that are linearly independent, form a unit cell with at
least a minimum volume and for which the corresponding :ref:`UB matrix <Lattice>`
indexes at least 80% of the maximum number of indexed using any set of three
vectors chosen from the list.

A :ref:`UB matrix <Lattice>` is formed using these three vectors and the resulting
:ref:`UB matrix <Lattice>` is again optimized using a least squares method. Finally,
starting from this matrix, a matrix corresponding to the Niggli reduced cell is
calculated and returned as the :ref:`UB matrix <Lattice>`. If the specified peaks
are accurate and belong to a single crystal, this method should produce the
:ref:`UB matrix <Lattice>` corresponding to the Niggli reduced cell. However, other
software will usually be needed to adjust this UB to match a desired
conventional cell. While this algorithm will occasionally work for as
few as four peaks, it works quite consistently with at least ten peaks,
and in general works best with a larger number of peaks.

Usage
-----

**Example:**

.. testcode:: ExFindUBUsingFFT

    ws=LoadIsawPeaks("TOPAZ_3007.peaks")
    print("After LoadIsawPeaks does the workspace have an orientedLattice: %s" % ws.sample().hasOrientedLattice())

    FindUBUsingFFT(ws,MinD=8.0,MaxD=13.0)
    print("After FindUBUsingFFT does the workspace have an orientedLattice: %s" % ws.sample().hasOrientedLattice())

    print(ws.sample().getOrientedLattice().getUB())


Output:

.. testoutput:: ExFindUBUsingFFT

    After LoadIsawPeaks does the workspace have an orientedLattice: False
    After FindUBUsingFFT does the workspace have an orientedLattice: True
    [[ 0.01223576  0.00480107  0.08604016]
     [-0.11654506  0.00178069 -0.00458823]
     [-0.02737294 -0.08973552 -0.02525994]]


.. categories::

.. sourcelink::
