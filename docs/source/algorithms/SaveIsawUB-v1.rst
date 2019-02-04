.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This saves a workspace's :ref:`UB matrix <Lattice>` to an ISAW-style UB matrix text file.

The resulting file can be loaded again into another workspace by using
the :ref:`algm-LoadIsawUB` algorithm.

The matrix saved is the transpose of the :ref:`UB matrix <Lattice>`. The
:ref:`UB matrix <Lattice>` maps the column vector (h,k,l ) to the column
vector (q'x,q'y,q'z). \|Q'\|=1/dspacing and its coordinates are a right-hand
coordinate system where x is the beam direction and z is vertically upward.
(IPNS convention)

Usage
-----

.. testcode:: SaveIsawUB

    #A workspace with some UB matrix
    w = CreateSingleValuedWorkspace()
    SetUB(w, 2, 4, 5, 90, 90, 90, "0,0,1", "1,0,0")
    #run the algorithm
    import mantid
    filename = mantid.config.getString("defaultsave.directory") + "saveIsawUBTest.mat"
    SaveIsawUB(w, filename)

    #check if the correct results are written in the file
    f = open(filename, "r")
    x, y, z = map(float, f.readline().split())
    if (x==0) and (y==0.5) and (z==0):
        print("The first line is 0 0.5 0")
    x, y, z = map(float, f.readline().split())
    if (x==0) and (y==0) and (z==0.25):
        print("The second line is 0 0 0.25")
    x, y, z = map(float, f.readline().split())
    if (x==0.2) and (y==0) and (z==0):
        print("The third line is 0.2 0 0")
    f.close()


.. testcleanup:: SaveIsawUB

   DeleteWorkspace('w')
   import os,mantid
   filename=mantid.config.getString("defaultsave.directory")+"saveIsawUBTest.mat"
   os.remove(filename)

Output:

.. testoutput:: SaveIsawUB

    The first line is 0 0.5 0
    The second line is 0 0 0.25
    The third line is 0.2 0 0

.. categories::

.. sourcelink::
