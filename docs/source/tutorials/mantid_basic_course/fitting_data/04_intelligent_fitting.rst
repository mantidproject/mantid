.. _04_intelligent_fitting:

===================
Intelligent Fitting
===================



Here we will tackle a more advanced aspect of Mantid fitting:
an Asymmetric peak!

Load the file GEM63437_focussed.nxs file and plot the *GEM63437_focussed_3*
workspace. Zoom into the region 4300 - 4900 microseconds (simply double-click on the X-axis and change the limits).

.. figure:: /images/GEMAsymmetricPeak.png
   :alt: GEMAsymmetricPeak.png
   :align: center

Try to fit it with a Gaussian (plus LinearBackground):

.. figure:: /images/GEMGaussianFit.png
   :alt: GEMGaussianFit.png
   :align: center

That's not a very good fit!

The Ikeda-Carpenter function is a better choice here.
But this is a difficult function to work with, it requires
very good initial parameter values for the fit to converge.

Mantid uses pre-set values in a different way for each instrument. For example, when the Ikeda-Carpenter function is applied on data from the GEM instrument, the fitting tools take this into account when setting the initial values, hopefully resulting in a better fit.

Right-click on the Gaussian function in the Fit Property Browser and remove it.
Now add the IkedaCarpenterPV function, check you still have a LinearBackground and run the Fit:

.. figure:: /images/GEMIkedaCarpenterFit.png
   :alt: GEMIkedaCarpenterFit.png
   :align: center
