.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This does a least squares fit between indexed peaks and Q values for a
set of runs producing an overall leastSquare orientation matrix.

Get estimates of the standard deviations of the parameters, by
approximating chisq by a quadratic polynomial through three points and
finding the change in the parameter that would cause a change of 1 in
chisq. (See Bevington, 2nd ed., pg 147, eqn: 8.13 ) In this version, we
calculate a sequence of approximations for each parameter, with delta
ranging over 10 orders of magnitude and keep the value in the sequence
with the smallest relative change.

Usage
-----

**Example:**

.. testcode:: ExOptimizeForCellType

    ws=LoadIsawPeaks("TOPAZ_3007.peaks")
    FindUBUsingFFT(ws,MinD=8.0,MaxD=13.0)
    print("Before Optimization:")
    print(ws.sample().getOrientedLattice().getUB())
    OptimizeLatticeForCellType(ws,CellType="Monoclinic")
    print("\nAfter Optimization:")
    print(ws.sample().getOrientedLattice().getUB())


Output:

.. testoutput:: ExOptimizeForCellType

    Before Optimization:
    [[ 0.01223576  0.00480107  0.08604016]
     [-0.11654506  0.00178069 -0.00458823]
     [-0.02737294 -0.08973552 -0.02525994]]

    After Optimization:
    [[-0.04519948  0.04084577 -0.01259083]
     [ 0.00160373 -0.00322317  0.11582993]
     [ 0.05743318  0.03223531  0.02753568]]

.. categories::

.. sourcelink::
