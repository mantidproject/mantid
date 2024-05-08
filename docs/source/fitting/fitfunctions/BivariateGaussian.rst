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

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    from scipy.stats import multivariate_normal
    import numpy as np
    import BivariateGaussian as BVG

    sigmaX = 0.05
    sigmaY = 0.15
    sigmaXY = 0.0
    muX = 0.1
    muY = 0.1
    p = 0.0
    covariance = np.matrix([[sigmaX*sigmaX, p*sigmaX*sigmaY],
                            [p*sigmaX*sigmaY,sigmaY*sigmaY]])
    rv = multivariate_normal([muX,muY], covariance)

    # Generate some data to fit
    x, y = np.mgrid[-0.5:0.5:.01, -0.5:0.5:.01]
    pos = np.dstack((x, y))
    Z = rv.pdf(pos)
    Z += 1.5*(np.random.random(x.shape) - 0.5) # Noise

    # Here we'll format it so we can fit this as a 1D function:
    ZForFitting = np.empty(Z.shape + (2,))
    ZForFitting[:,:,0] = Z
    ZForFitting[:,:,1] = Z

    pos = np.empty(x.shape + (2,))
    pos[:, :, 0] = x
    pos[:, :, 1] = y
    bvgWS = CreateWorkspace(OutputWorkspace='bvgWS', DataX=pos.ravel(
            ), DataY=ZForFitting.ravel())

    # Now we declare our function - we create one separately
    # so that later we have access to functions which are not
    # accessible to Function1D (e.g. 2D and 3D versions of the
    # function.  The parameters we set here are an initial guess.
    bvg = BVG.BivariateGaussian()
    bvg.init()
    bvg['A'] = 1.
    bvg['SigX'] = 0.05
    bvg['SigY'] = 0.15
    bvg['SigP'] = 0.
    bvg['MuX']=0.1
    bvg['MuY']=0.1
    bvg.setAttributeValue('nX', Z.shape[0])
    bvg.setAttributeValue('nY', Z.shape[1])
    Fit(Function=bvg, InputWorkspace='bvgWS', Output='bvgfit',
                            Minimizer='Levenberg-MarquardtMD')

    ZFit = bvg.function2D(pos)

    #Plot the results
    fig, axes = plt.subplots(nrows=1, ncols=2, subplot_kw={'projection': 'mantid'})
    axes[0].imshow(Z, origin='lower')
    axes[0].set_title('Data')
    axes[1].imshow(ZFit, origin='lower')
    axes[1].set_title('Fit')
    fig.show()


.. categories::

.. sourcelink::
