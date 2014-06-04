.. algorithm::

.. summary::

.. alias::

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
  x = np.linspace(-10,10,100)
  y = 1.0 / (x**2 + 10)
  e = np.ones_like(x)
  ws = CreateWorkspace( x, y, e )

  # Run algorithm with low number of spline coefficients
  out = SplineBackground(ws, WorkspaceIndex=0, NCoeff=4)
  yout = out.readY(0)
  print 'Fit quality is',np.sum( (y - yout)**2 )

  # Increase the number of spline coefficients
  out = SplineBackground(ws, WorkspaceIndex=0, NCoeff=20)
  yout = out.readY(0)
  print 'Fit quality is',np.sum( (y - yout)**2 )

.. testoutput:: ExampleSpline

  Fit quality is 0.0253975992834
  Fit quality is 1.53267607188e-07


.. categories::
