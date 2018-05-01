.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithms will attach an OrientedLattice object to a sample in the
workspace. For MD workspaces, you can select to which sample to attach
it. If nothing entered, it will attach to all. If bad number is
enetered, it will attach to first sample.

If :ref:`UB matrix <Lattice>` elements are entered, lattice parameters and orientation
vectors are ignored. The algorithm will throw an exception if the
determinant is 0. If the :ref:`UB matrix <Lattice>` is all zeros (default), it will
calculate it from lattice parameters and orientation vectors (MSlice and Horace style).
The algorithm will throw an exception if u and v are collinear, or one of
them is very small in magnitude.


Usage
-----

.. testcode:: SetUB

    # create a workspace (or you can load one)
    ws = CreateSingleValuedWorkspace(5)

    #set a UB matrix using the vector along k_i as 1,1,0, and the 0,0,1 vector in the horizontal plane
    SetUB(ws, a=5, b=6, c=7, alpha=90, beta=90, gamma=90, u="1,1,0", v="0,0,1")

    #check that it works
    from numpy import array, array_str
    mat = array(ws.sample().getOrientedLattice().getUB())
    print("UB matrix")
    print(array_str(mat, precision=3, suppress_small=True))

.. testcleanup:: SetUB

    DeleteWorkspace(ws)


Output:

.. testoutput:: SetUB
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

    UB matrix
    [[...     ...     0.143]
     [ 0.128 -0.128  ...   ]
     [ 0.154  0.107 ...   ]]


.. categories::

.. sourcelink::
