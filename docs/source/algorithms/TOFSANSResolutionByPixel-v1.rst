.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates the Q-resolution per pixel according to Mildner and Carpenter equation

.. math:: (\sigma_Q )^2 = \frac{4\pi^2}{12\lambda^2} [ 3(\frac{R_1}{L_1})^2 + 3(\frac{R_2}{L_3})^2 + (\frac{\Delta R}{L_2})^2 ] + Q^2(\frac{\sigma_{\lambda}}{\lambda})^2

where :math:`L1` and :math:`L2`  are the collimation length and sample-to-detector distance respectively and 

.. math:: \frac{1}{L_3} = \frac{1}{L_1} + \frac{1}{L_2}

and

.. math:: (\sigma_{\lambda})^2 = (\Delta \lambda )^2 / 12 + (\sigma_{moderator})^2

where :math:`\sigma_{\lambda}` is the overall wavelength std from TOF binning
and moderator, :math:`\Delta \lambda` is taken from the binning of the InputWorkspace 
and the :math:`\sigma_{moderator}` is the wavelenght spread from the moderator.

where :math:`\sigma_{\lambda}` is the effective standard deviation, and :math:`\Delta \lambda`,
originating from the TOF binning of the InputWorkspace, is the (rectangular)
width, of the moderator wavelength distribution. :math:`\sigma_{moderator}` is the
moderator time spread (the variation in time for the moderator to emit neutrons
of a given wavelength).

:math:`\sigma_Q` is returned as the y-values of the InputWorkspace, and the 
remaining variables in the main equation above are related to parameters of this
algorithm as follows:

* :math:`R_1` equals SourceApertureRadius
* :math:`R_2` equals SampleApertureRadius
* :math:`\Delta R` equals DeltaR
* :math:`\sigma_{moderator}` equals SigmaModerator  
* :math:`\L_1` equals CollimationLength

:math:`\lambda` in the equation is the midpoint of wavelength 
histogram bin values of InputWorkspace.

Collimation length :math:`L_1` in metres in the equation here is the distance between the
first beam defining pinhole (Radius :math:`R_1`) and the sample aperture (radius :math:`R_2`).
(Beware that :math:`L_1` is more often the moderator to sample distance.)
 
For rectangular collimation apertures, size H x W, Mildner & Carpenter say to
use :math:`R = \sqrt{( H^2 +W^2)/6 }`. Note that we are assuming isotropically averaged,
scalar :math:`Q`, and making some small angle approximations. Results on higher angle detectors
may not be accurate. For data reduction sliced in different directions on the detector
(e.g. GISANS) adjust the calling parameters to suit the collimation in that direction.

This version of the algorithm neglects wavelength-dependent detector detection depth effects.

.. categories::

.. sourcelink::
