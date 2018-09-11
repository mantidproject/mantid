.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

SplineBackground uses GSL b-spline and fitting functions to fit a
spectrum. Masked bins are excluded from the fit making it possible to
fit only the background signal. The output workspace has one spectrum of
calculated values and the fitting errors.

Usage
-----

.. testcode:: ExampleSpline

   import numpy as np

   # Create a workspace with some data

   # Fill array x with 100 float values equally spaced in the interval [-10, 10]
   x = np.linspace(-10,10,100)
   # Fill array y with values of a function for each number in array x. y has the same length as x
   y = 1.0 / (x**2 + 10)
   # Create array e of the same length as x and fill it with ones (1.0)
   e = np.ones_like(x)
   # Uses the above arrays to create a workspace
   ws = CreateWorkspace( x, y, e )

   # Run algorithm with low number of spline coefficients
   out = SplineBackground(ws, WorkspaceIndex=0, NCoeff=4)
   yout = out.readY(0)
   # Sum the squares of the differences of elements of arrays y and yout
   print('Fit quality is {}'.format(np.sum( (y - yout)**2 )))

   # Increase the number of spline coefficients
   out = SplineBackground(ws, WorkspaceIndex=0, NCoeff=20)
   yout = out.readY(0)
   # Sum the squares of the differences of elements of arrays y and yout
   print('Fit quality is {}'.format(np.sum( (y - yout)**2 )))

.. testoutput:: ExampleSpline
   :options: +ELLIPSIS

   Fit quality is ...
   Fit quality is ...


.. categories::

.. sourcelink::
