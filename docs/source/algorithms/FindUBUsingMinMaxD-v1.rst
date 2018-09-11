.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a set of peaks, and given a range of possible a,b,c values, this
algorithm will attempt to find a :ref:`UB matrix <Lattice>`, corresponding
to the `Niggli reduced cell <http://nvlpubs.nist.gov/nistpubs/sp958-lide/188-190.pdf>`__,
that fits the data. The algorithm searches over a range of possible
directions and unit cell lengths for directions and lengths that match
plane normals and plane spacings in reciprocal space. It then chooses
three of these vectors with the shortest lengths that are linearly
independent and that are separated by at least a minimum angle. An
initial :ref:`UB matrix <Lattice>` is formed using these three vectors
and the resulting :ref:`UB matrix <Lattice>` is optimized using a least
squares method. Finally, starting from this matrix, a matrix corresponding
to the Niggli reduced cell is calculated and returned as the
:ref:`UB matrix <Lattice>`. If the specified peaks are accurate and belong
to a single crystal, this method should produce some :ref:`UB matrix <Lattice>`
that indexes the peaks. However, other software will usually be needed to
adjust this UB to match a desired conventional cell.

Usage
-----

**Example:**

.. testcode:: ExFindUBUsingMinMaxD

    ws=LoadIsawPeaks("TOPAZ_3007.peaks")
    print("After LoadIsawPeaks does the workspace have an orientedLattice: %s" % ws.sample().hasOrientedLattice())

    FindUBUsingMinMaxD(ws,MinD=8.0,MaxD=13.0)
    print("After FindUBUsingMinMaxD does the workspace have an orientedLattice: %s" % ws.sample().hasOrientedLattice())

    print(ws.sample().getOrientedLattice().getUB())


Output:

.. testoutput:: ExFindUBUsingMinMaxD

    After LoadIsawPeaks does the workspace have an orientedLattice: False
    After FindUBUsingMinMaxD does the workspace have an orientedLattice: True
    [[ 0.01223576  0.00480107  0.08604016]
     [-0.11654506  0.00178069 -0.00458823]
     [-0.02737294 -0.08973552 -0.02525994]]


.. categories::

.. sourcelink::
