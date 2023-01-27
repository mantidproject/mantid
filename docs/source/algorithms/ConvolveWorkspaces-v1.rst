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

    Output:  [  2.94086052e-03   4.94138455e-03   7.88731177e-03   1.20544038e-02
       1.77602905e-02   2.53593470e-02   3.52218371e-02   4.76946980e-02
       6.30473428e-02   8.14098249e-02   1.02712190e-01   1.26634120e-01
       1.52573963e-01   1.79645656e-01   2.06709925e-01   2.32442090e-01
       2.55433130e-01   2.74314507e-01   2.87891918e-01   2.95270020e-01
       2.95950061e-01   2.89885648e-01   2.77487984e-01   2.59579748e-01
       2.37304811e-01   2.12007552e-01   1.85099436e-01   1.57931131e-01
       1.31685868e-01   1.07304826e-01   8.54491718e-02   6.64974252e-02
       5.05719855e-02   3.75857483e-02   2.72988420e-02   1.93763900e-02
       1.34402982e-02   9.11070742e-03   6.03534645e-03   3.90713983e-03
       2.47184976e-03   1.52823888e-03   9.23349438e-04   5.45187551e-04
       3.14578500e-04   1.77382260e-04   9.77402935e-05   5.26229816e-05
       2.76760575e-05   1.42103909e-05]

.. categories::

.. sourcelink::
