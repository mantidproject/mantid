.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Convolution of two workspaces using the :ref:`Convolution <func-Convolution>` function. Workspaces must have the same number of spectra.

Usage
-----

**Example: Convolve sample workspaces**

.. testcode:: ExConvolveWorkspaces


    from numpy import ndarray
    from math import pi

    x = np.linspace(-5,5)
    f1 = Gaussian(Height=1., PeakCentre=.3, Sigma=.6)
    # normalise the gaussian
    y1 = f1(x)/(0.6*np.sqrt(2.*pi))
    ws1 = CreateWorkspace(x, y1)

    f2 = Gaussian(Height=1., PeakCentre=-1.3, Sigma=1.2)
    # normalise the gaussian
    y2 = f2(x)/(1.2*np.sqrt(2.*pi))
    ws2 = CreateWorkspace(x, y2)

    # calculate convolution
    ws3 = ConvolveWorkspaces(ws1, ws2)

    print("Output:  {}".format(ws3.readY(0)))

Output:

.. testoutput:: ExConvolveWorkspaces

    Output:  [... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ... ... ...
     ... ...]

.. categories::

.. sourcelink::
