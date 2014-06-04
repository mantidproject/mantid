.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithms will attach an OrientedLattice object to a sample in the
workspace. For MD workspaces, you can select to which sample to attach
it. If nothing entered, it will attach to all. If bad number is
enetered, it will attach to first sample.

If UB matrix elements are entered, lattice parameters and orientation
vectors are ignored. The algorithm will throw an exception if the
determinant is 0. If the UB matrix is all zeros (default), it will
calculate it from lattice parameters and orientation vectors. The
algorithm will throw an exception if u and v are collinear, or one of
them is very small in magnitude.

.. categories::
