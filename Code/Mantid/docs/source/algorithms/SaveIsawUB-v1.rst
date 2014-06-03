.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This saves a workspace's UB matrix to an ISAW-style UB matrix text file.

The resulting file can be loaded again into another workspace by using
the :ref:`algm-LoadIsawUB` algorithm.

The matrix saved is the transpose of the UB Matrix. The UB matrix maps
the column vector (h,k,l ) to the column vector (q'x,q'y,q'z).
\|Q'\|=1/dspacing and its coordinates are a right-hand coordinate system
where x is the beam direction and z is vertically upward. (IPNS
convention)

.. categories::
