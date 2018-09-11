.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a PeaksWorkspace with a :ref:`UB matrix <Lattice>` stored with
the sample, this algoritm will accept a 3x3 transformation matrix M,
change UB to UB\*M-inverse and map each (HKL) vector to M\*(HKL). For
example, the transformation with elements 0,1,0,1,0,0,0,0,-1 will
interchange the H and K values and negate L. This algorithm should allow
the usr to perform any required transformation of the Miller indicies,
provided that transformation has a positive determinant. If a transformation
with a negative or zero determinant is entered, the algorithm with throw an
exception. The 9 elements of the transformation must be specified as a
comma separated list of numbers.

Usage
-----

**Example:**

.. testcode:: ExTransformHKL

    ws=LoadIsawPeaks("TOPAZ_3007.peaks")
    FindUBUsingFFT(ws,MinD=8.0,MaxD=13.0)

    print("Before Transformation:")
    print(ws.sample().getOrientedLattice().getUB())

    #This HKLTransform is a matrix that will swap H and K and negate L
    TransformHKL(ws,HKLTransform="0,1,0,1,0,0,0,0,-1")
    print("\nAfter Transformation:")
    print(ws.sample().getOrientedLattice().getUB())


Output:

.. testoutput:: ExTransformHKL

    Before Transformation:
    [[ 0.01223576  0.00480107  0.08604016]
     [-0.11654506  0.00178069 -0.00458823]
     [-0.02737294 -0.08973552 -0.02525994]]

    After Transformation:
    [[ 0.00480107  0.01223576 -0.08604016]
     [ 0.00178069 -0.11654506  0.00458823]
     [-0.08973552 -0.02737294  0.02525994]]


.. categories::

.. sourcelink::
