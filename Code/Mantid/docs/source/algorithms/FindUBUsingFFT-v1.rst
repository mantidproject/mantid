.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given a set of peaks, and given a range of possible a,b,c values, this
algorithm will attempt to find a UB matrix, corresponding to the Niggli
reduced cell, that fits the data. The algorithm projects the peaks on
many possible direction vectors and calculates a Fast Fourier Transform
of the projections to identify regular patterns in the collection of
peaks. Based on the calcuated FFTs, a list of directions corresponding
to possible real space unit cell edge vectors is formed. The directions
and lengths of the vectors in this list are optimized (using a least
squares approach) to index the maximum number of peaks, after which the
list is sorted in order of increasing length and duplicate vectors are
removed from the list.

The algorithm then chooses three of the remaining vectors with the
shortest lengths that are linearly independent, form a unit cell with at
least a minimum volume and for which the corresponding UB matrix indexes
at least 80% of the maximum number of indexed using any set of three
vectors chosen from the list.

A UB matrix is formed using these three vectors and the resulting UB
matrix is again optimized using a least squares method. Finally,
starting from this matrix, a matrix corresponding to the Niggli reduced
cell is calculated and returned as the UB matrix. If the specified peaks
are accurate and belong to a single crystal, this method should produce
the UB matrix corresponding to the Niggli reduced cell. However, other
software will usually be needed to adjust this UB to match a desired
conventional cell. While this algorithm will occasionally work for as
few as four peaks, it works quite consistently with at least ten peaks,
and in general works best with a larger number of peaks.

.. categories::
