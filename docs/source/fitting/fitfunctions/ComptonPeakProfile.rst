.. _func-ComptonPeakProfile:

===================
ComptonPeakProfile
===================

.. index:: ComptonPeakProfile

Description
-----------

The ComptonPeakProfile describes the Neutron Compton profile with either a Voigt or Gaussian approximation,
depending on an energy cutoff value. It takes three input parameters:

-  Intensity: :math:`I`
-  Position: :math:`P`
-  SigmaGauss: :math:`\sigma_G`

and three attributes,

-  WorkspaceIndex
-  Mass
-  VoigtEnergyCutOff

The VoigtEnergyCutOff is used to determine whether a Gaussian or Voigt function is used to fit the peak. If
the final energy, as read from the input workspace, is greater than the VoigtEnergyCutOff a normalised Gaussian approximation is used,

.. math:: \frac{I}{2\pi \sigma_T^2}\exp \left( -0.5*\frac{(x-P)^2}{\sigma_T^2} \right)

where :math:`\sigma_T^2` is the total variance, defined by

.. math:: \sigma_T^2 = \sigma_G^2 + \Gamma^2

where :math:`\Gamma` is the half-width-half-maximum, estimated from the input data.

If instead, the energy is below the cutoff an :ref:`approximation <func-Voigt>` to a Voigt function is used with the following inputs:

.. math:: \text{LorentzAmp} = I
.. math:: \text{LorentzPos} = P
.. math:: \text{LorentzFWHM} = 2\Gamma
.. math:: \text{GaussianFWHM}= 0.5 \sigma_G \sqrt{\ln(4)}


.. attributes::

.. properties::

.. categories::

.. sourcelink::
