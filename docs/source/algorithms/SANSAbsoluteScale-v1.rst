.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculate and apply absolute scale correction for SANS data.
The method used can either be a simple scaling using the *ScalingFactor* property, or
computing the scaling factor using a reference data set. In this case, the reference
data is loaded, normalized and corrected for detector sensitivity. The corrected reference
data is then used to compute the scaling factor according to

:math:`f = \frac{N_b}{M*T} ( d/L )^2`

where :math:`N_b` is the total count within the distance of a beam diameter of the beam center.

:math:`M` is the monitor count. 

:math:`T` is the attenuator transmission. 

:math:`d` is the detector pixel width. 

:math:`L` is the sample-detector distance. 

The input workspace is then scaled by :math:`1/f`.

This algorithm is rarely called directly. It is called by 
:ref:`HFIRSANSReduction <algm-HFIRSANSReduction>`.

.. categories::

.. sourcelink::
