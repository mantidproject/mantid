.. _03_intelligent_fitting:

===================
Intelligent Fitting 
===================

Intelligent Fitting
===================

Fitting is the modelling of data where parameters of the model are
allowed to vary during a fitting process until the model and data agree
better according to some cost function.

In summary the Mantid fitting provides

-  General fitting capabilities
-  Neutron and Muon intelligent fitting tools, which makes use of
   additional information about the data, such as instrument geometry
   and log value information
-  Easy expandability

Here a more advanced aspect of Mantid fitting is presented. We will fit
an asymmetric peak from a GEM data set with the Ikeda-Carpenter function
on a LinearBackground.

| Let's load GEM63437_focussed.nxs file and plot GEM63437_focussed_3
  workspace. Zoom into the region 4300 - 4900 microseconds.
| |GEMAsymmetricPeak.png|

| Try to fit it with a Gaussian (plus LinearBackground):
| |GEMGaussianFit.png|

Not a very good job. The Ikeda-Carpenter function is a better choice
here. But this is a very difficult function to work with. It requires
very good initial parameter values for the fit to converge. The Mantid
approach is to use the pre-set values which are defined on a
per-instrument basis. For example, when the Ikeda-Carpenter is used with
GEM data the fitting tools automatically find and set the appropriate
initial values. This results in a good fit.

.. figure:: /images/GEMIkedaCarpenterFit.png
   :alt: GEMIkedaCarpenterFit.png
   :width: 400px

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Fit_Model_Choices |Mantid_Basic_Course|MBC_Exercise_Intelligent_Fitting}}

.. |GEMAsymmetricPeak.png| image:: /images/GEMAsymmetricPeak.png
   :width: 400px
.. |GEMGaussianFit.png| image:: /images/GEMGaussianFit.png
   :width: 400px
