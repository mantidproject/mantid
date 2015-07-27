.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm constructs a :ref:`MatrixWorkspace <MatrixWorkspace>`
when passed a vector for each of the X, Y, and E data values. The unit
for the X Axis can optionally be specified as any of the units in the
Mantid `Unit Factory <http://www.mantidproject.org/Units>`__ (see `the
list of units currently available
<http://www.mantidproject.org/Units>`__).  Multiple spectra may be
created by supplying the NSpec Property (integer, default 1). When
this is provided the vectors are split into equal-sized spectra (all
X, Y, E values must still be in a single vector for input).

When you use the input property ParentWorkspace, the new workspace is
created with the same instrument (including its parameters), sample
and run information, and comment field as the parent. The Y units and
title are also copied from the parent workspace unless they are
provided in the input properties passed to the algorithm.

Usage
-----

.. testcode::

     dataX = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
     dataY = [1,2,3,4,5,6,7,8,9,10,11,12]
     dataE = [1,2,3,4,5,6,7,8,9,10,11,12]
     
     # The workspace will be named "dataWS"
     dataWS = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=4,UnitX="Wavelength")

.. categories::

.. sourcelink::
