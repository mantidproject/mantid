.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a set of peaks at least three of which have been assigned Miller
indices, this algorithm will find the :ref:`UB matrix <Lattice>`, that
best maps the integer (h,k,l) values to the corresponding Q vectors. The set of
indexed peaks must include three linearly independent Q vectors. The
(h,k,l) values from the peaks are first rounded to form integer (h,k,l)
values. The algorithm then forms a possibly over-determined linear
system of equations representing the mapping from (h,k,l) to Q for each
indexed peak. The system of linear equations is then solved in the least
squares sense, using QR factorization.

Usage
-----

**Example:**

.. testcode:: ExFindUBUsingIndexedPeaks

    ws=LoadIsawPeaks("TOPAZ_3007.peaks")
    print("After LoadIsawPeaks does the workspace have an orientedLattice: %s" % ws.sample().hasOrientedLattice())

    FindUBUsingIndexedPeaks(ws)
    print("After FindUBUsingIndexedPeaks does the workspace have an orientedLattice: %s" % ws.sample().hasOrientedLattice())

    print(ws.sample().getOrientedLattice().getUB())


Output:

.. testoutput:: ExFindUBUsingIndexedPeaks

    After LoadIsawPeaks does the workspace have an orientedLattice: False
    After FindUBUsingIndexedPeaks does the workspace have an orientedLattice: True
    [[-0.04542062  0.04061954 -0.01223576]
     [ 0.00140377 -0.00318446  0.11654506]
     [ 0.05749773  0.03223779  0.02737294]]


.. categories::

.. sourcelink::
