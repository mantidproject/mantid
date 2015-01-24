.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates the Q-resolution per pixel according to Mildner and Carpenter equation

.. math:: (\sigma_Q )^2 = \frac{4\pi^2}{12\lambda^2} [ 3(\frac{R_1}{L_1})^2 + 3(\frac{R_2}{L_3})^2 + (\frac{\Delta R}{L_2})^2 ] + Q^2(\frac{\sigma_{\lambda}}{\lambda})^2

where :math:`L1` and :math:`L2` are the primary and secondary flight-paths respectively and 

.. math:: \frac{1}{L_3} = \frac{1}{L_1} + \frac{1}{L_2}

and

.. math:: (\sigma_{\lambda})^2 = (\Delta \lambda )^2 / 12 + (\sigma_{moderator})^2

where :math:`\sigma_{\lambda}` is the overall wavelength std from TOF binning
and moderator, :math:`\Delta \lambda` is taken from the binning of the InputWorkspace 
and the :math:`\sigma_{moderator}` is the wavelenght spread from the moderator.

:math:`\sigma_Q` is returned as the y-values of the InputWorkspace, and the 
remaining variables in the main equation above are related to parameters of this
algorithm as follows:

* :math:`R_1` equals SourceApertureRadius
* :math:`R_2` equals SampleApertureRadius
* :math:`\Delta R` equals DeltaR
* :math:`\sigma_{moderator}` equals SigmaModerator  

:math:`\lambda` in the equation is the midtpoint of wavelength 
histogram bin values of InputWorkspace.

.. categories::
