.. _func-BivariateGaussian:

===================
BivariateGaussian
===================

.. index:: BivariateGaussian

Description
-----------

This function provides a Bivariate Gaussian distribution by acting as a wrapper to matplotlib.mlab.bivariate_normal.  
It differs from :ref:`func-BivariateNormal` in that it does not require any detector-space-specific input and 
allows for a correlation term.

.. math:: V= \frac{A}{2 \pi \sigma_x \sigma_y \sqrt{1-\rho^2}} \times \exp \bigg[- \frac{1}{2(1-\rho^2)} \bigg( \frac{(x-\mu_x)^2}{\sigma_x^2} + \frac{(y-\mu_y)^2}{\sigma_y^2}  - \frac{2 \rho (x-\mu_x) (y-\mu_y)} {\sigma_x \sigma_y} \bigg) \bigg] + bg

.. attributes::

The attributes **nX** and **nY** store the dimension of the two histogram.  This is useful when reconstructing 
a 2D histogram from a fit, which reduces the data 1D.

.. properties::

These are standard properties for a bivariate Gaussian distribution.  See the equation above.

Usage
~~~~~

Here is an example of fitting a 2D histogram:

.. code-block:: python
    :linenos:

    from matplotlib.mlab import bivariate_normal
    import numpy as np
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import BivariateGaussian as BVG

    # Generate some data to fit
    x = np.linspace(-0.5,0.5,100)
    y = np.linspace(-0.5,0.5,100)
    X,Y = np.meshgrid(x,y,indexing='ij')
    sigmaX = 0.05
    sigmaY = 0.15
    sigmaXY = 0.0
    muX = 0.1
    muY = 0.1
    Z = 1.0*bivariate_normal(X, Y, sigmax=sigmaX, sigmay=sigmaY,
                                 sigmaxy=sigmaXY, mux=muX, muy=muY)
    Z += 0.1*(np.random.random(X.shape) - 0.5) # Noise 

    # Here we'll format it so we can fit this as a 1D function:
    ZForFitting = np.empty(Z.shape + (2,))
    ZForFitting[:,:,0] = Z
    ZForFitting[:,:,1] = Z

    pos = np.empty(X.shape + (2,))
    pos[:, :, 0] = X
    pos[:, :, 1] = Y
    bvgWS = CreateWorkspace(OutputWorkspace='bvgWS', DataX=pos.ravel(
            ), DataY=ZForFitting.ravel())

    # Now we declare our function - we create one separately
    # so that later we have access to functions which are not 
    # accessible to Function1D (e.g. 2D and 3D versions of the
    # function.  The paramters we set here are an initial guess.
    bvg = BVG.BivariateGaussian()
    bvg.init()
    bvg['A'] = 1.
    bvg['sigX'] = 0.1
    bvg['sigY'] = 0.1
    bvg['sigP'] = 0.
    bvg.setAttributeValue('nX', Z.shape[0])
    bvg.setAttributeValue('nY', Z.shape[1])
    Fit(Function=bvg, InputWorkspace='bvgWS', Output='bvgfit',
                             Minimizer='Levenberg-MarquardtMD')
                             
    ZFit = bvg.function2D(pos)

    #Plot the results
    plt.figure(1)
    plt.clf()
    plt.subplot(1,2,1)
    plt.imshow(Z)
    plt.title('Data')
    plt.subplot(1,2,2)
    plt.imshow(ZFit)
    plt.title('Fit')




.. categories::

.. sourcelink::
