.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads the UB matrix into a workspace from an ISAW-style UB matrix ASCII
file.

You can use the :ref:`algm-SaveIsawUB` algorithm to save to this
format.

The matrix in the file is the transpose of the UB Matrix. The UB matrix
maps the column vector (h,k,l ) to the column vector (q'x,q'y,q'z).
\|Q'\|=1/dspacing and its coordinates are a right-hand coordinate system
where x is the beam direction and z is vertically upward. (IPNS
convention)

Note: for an MDEventWorkspace, all experimentInfo objects will contain
teh oriented lattice loaded from the IsawUB file

.. categories::
