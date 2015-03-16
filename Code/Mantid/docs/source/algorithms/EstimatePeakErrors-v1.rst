.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a function after it has been optimized by the Fit algorithm and calculates peak parameter and associated errors for all peaks in this function. The peak parameters are its centre, height, FWHM and intensity. The output workspace is a table with three columns: parameter name, parameter value and parameter error.

Usage
-----
(*At the moment the algorithm works propertly if run from C++ only.*)
#.. testcode::

    import numpy as np

    # Create a data set
    x = np.linspace(-10,10,100)
    y = 10 * 2.0 / (2.0**2 + (x+4)**2 ) + 10 * 3.0 / (3.0**2 + (x-3)**2 ) + 3.0
    e = np.ones_like(x)
    ws = CreateWorkspace(x,y,e)

    # Define a fitting function.
    fun = "name=Lorentzian,Amplitude=10,PeakCentre=-4,FWHM=2;"+\
                  "name=Lorentzian,Amplitude=10,PeakCentre=3,FWHM=3;"+\
                  "name=FlatBackground,A0=3"

    # Fit the function.
    Fit(fun,ws)

    # Calculate peak parameter error estimates for the two Lorentzians.
    params = EstimatePeakErrors(fun)

 
.. categories::
