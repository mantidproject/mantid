.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm constructs a MatrixWorkspace when passed a vector for each
of the X, Y, and E data values. The unit for the X Axis can optionally be
specified as any of the units in the Kernel's UnitFactory.  Multiple spectra
may be created by supplying the NSpec Property (integer, default 1). When
this is provided the vectors are split into equal-sized spectra (all X, Y,
E values must still be in a single vector for input).

Usage
-----

.. testcode::

     dataX = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
     dataY = [1,2,3,4,5,6,7,8,9,10,11,12]
     dataE = [1,2,3,4,5,6,7,8,9,10,11,12]
     
     # The workspace will be named "dataWS"
     dataWS = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=dataE, NSpec=4,UnitX="Wavelength")

.. categories::
