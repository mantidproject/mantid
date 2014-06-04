.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given a set of peaks, and given a range of possible a,b,c values, this
algorithm will attempt to find a UB matrix, corresponding to the `Niggli
reduced
cell <http://nvlpubs.nist.gov/nistpubs/sp958-lide/188-190.pdf>`__, that
fits the data. The algorithm searches over a range of possible
directions and unit cell lengths for directions and lengths that match
plane normals and plane spacings in reciprocal space. It then chooses
three of these vectors with the shortest lengths that are linearly
independent and that are separated by at least a minimum angle. An
initial UB matrix is formed using these three vectors and the resulting
UB matrix is optimized using a least squares method. Finally, starting
from this matrix, a matrix corresponding to the Niggli reduced cell is
calculated and returned as the UB matrix. If the specified peaks are
accurate and belong to a single crystal, this method should produce some
UB matrix that indexes the peaks. However, other software will usually
be needed to adjust this UB to match a desired conventional cell.

.. categories::
