.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a PeaksWorkspace with a :ref:`UB matrix <Lattice>` stored with
the sample, this algorithm will accept a 3x3 transformation matrix :math:`M`,
change :math:`UB` to :math:`UBM^{-1}` and map each :math:`(HKL)` vector to :math:`M(HKL)`.
For example, the transformation with elements 0,1,0,1,0,0,0,0,-1 will
interchange the :math:`H` and :math:`K` values and negate :math:`L`.
This algorithm should allow
the user to perform any required transformation of the Miller indices,
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

**Example - skipping error calculation**

In special cases, users may not have enough peaks to find the error.
For example, if peaks are constrained to two dimensions in HKL space,
the error calculation will fail. In this case, `FindError=False` can be
specified to skip the calculation. In this case, the lattice parameter
error will be set to zero.

.. code-block:: python

    ws = CreatePeaksWorkspace(OutputType='LeanElasticPeak', NumberOfPeaks=0)

    SetUB(ws, 5, 6, 7, 90, 90, 120)

    ws.addPeak(ws.createPeakHKL([1, 2, 0]))
    ws.addPeak(ws.createPeakHKL([0, 0, 3]))

    TransformHKL(ws, tolerance=0.15, HKLTransform='0,1,0,1,0,0,0,0,-1', FindError=False)

    print('peak(0) = ', ws.getPeak(0).getHKL())
    print('peak(1) = ', ws.getPeak(1).getHKL())

    ol = ws.sample().getOrientedLattice()
    print('ea = ', ol.errora())
    print('eb = ', ol.errorb())
    print('ec = ', ol.errorc())
    print('ealpha = ', ol.erroralpha())
    print('ebeta = ', ol.errorbeta())
    print('egamma = ', ol.errorgamma())

gives

.. code-block:: text

    peak(0) =  [2,1,0]
    peak(1) =  [0,0,-3]
    ea =  0.0
    eb =  0.0
    ec =  0.0
    ealpha =  0.0
    ebeta =  0.0
    egamma =  0.0

.. categories::

.. sourcelink::
