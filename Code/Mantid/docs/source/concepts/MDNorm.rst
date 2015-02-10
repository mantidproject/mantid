.. _MDNorm:

Multi dimensional neutron scattering data normalization
=======================================================

Introduction to normalization
+++++++++++++++++++++++++++++

In any experiment, a measurement consists of a raw quantity of interest and
a statistical significance of the measurement itself. 
For neutron diffraction, the differential scattering cross section at some 
point :math:`\mathbf{Q}` in the reciprocal space, measured with a single
detector with a solid angle :math:`d \Omega`, is given by:

.. math::
    :label: CrossSectionSingDet

    \frac{d\sigma}{d\Omega}=\frac{N}{\Phi \times d\Omega}
    
where :math:`N` is the number of scattered neutrons in a small volume 
:math:`d \mathbf{Q}` around :math:`\mathbf{Q}`, and :math:`\Phi` is
the time integrated incident flux that contribute to the scattering in the given
volume. :math:`N` is the raw quantity, while :math:`\Phi \times d \Omega` 
is the statistical significance, or norm.

If there are multiple detectors, or multiple experiments contributing to the 
scattering in the :math:`d \mathbf{Q}` volume, one needs to add together the raw
data, add together the norms, and then divide

.. math::
    :label: CrossSectionMultiDet
    
    \frac{d\sigma}{d\Omega}=\frac{\sum_i N_i}{\sum_i \Phi_i \times d\Omega_i}
  
The summation index :math:`i` represents every detector and sample orientation 
or repeated measurement that contribute to the scattering in the desired region 
of the reciprocal space. 
In a similar fashion, for inelastic scattering, the double
differential cross section must be written as:

.. math::
    :label: CrossSectionInelastic

    \frac{d^2 \sigma}{dE d\Omega}=\frac{\sum_i N_i}{\sum_i \Phi_i \times d\Omega_i \times dE_i}

What this means is that, in the triple axes type
of measurements for example, where we have a single detector (solid
angle :math:`d \Omega_i` is a constant), we should not 
normalize data by monitor counts and then add different experiments together. The monitor
count is a proxy for the incident flux. We should instead
add raw data together, add monitors together, and only then divide.
  
.. important:

   To correctly account for the statistical significance of the measurement
   always carry around separately the raw data and the normalization.

Detector trajectories in reciprocal space for single crystal experiments
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   
For direct geometry inelastic scattering, for any given experiment, all the incident
flux :math:`\Phi_i`  contributes to the scattering, and it is just a number. 
For diffraction and indirect geometry inelastic experiments one has to 
account only for the flux that contribute to the scattering in the :math:`d \mathbf{Q}`
region, which is detector and momentum dependent. Similarly, :math:`dE_i` is the length
along energy transfer axis of the detector trajectory inside the :math:`d \mathbf{Q}`
region. It is therefore important to understand where is the scattering in reciprocal space
for each of the detectors. In this section we describe the case of single crystal experiments.
We assume that the regions :math:`d \mathbf{Q}` are given by a regular gridding
of the data in reciprocal space.

For a scattering event in a particular detector,
the momentum transfer in the laboratory frame is related to the momentum transfer 
in the sample frame by the rotation of the sample goniometer. This is further related to 
the crystallographic :math:`HKL` frame by the :math:`UB` matrix. In Mantid notation
this can be written as

.. math::
    :label: MasterEquation

    \left(\begin{array}{r}
        -k_F \sin(\theta) \cos(\phi)\\
        -k_F \sin(\theta) \sin(\phi)\\
        k_I - k_F \cos(\theta) 
    \end{array}\right) = 
    R \left(\begin{array}{c}
        Q^{sample}_x \\
        Q^{sample}_y \\
        Q^{sample}_z         
    \end{array}\right) =
    2 \pi R \cdot U \cdot B 
    \left(\begin{array}{c}
        H \\
        K \\
        L         
    \end{array}\right)
    
where :math:`k_I` is the momentum of the incident neutron and :math:`k_F` is the one
of the scattered neutron. R is the rotation matrix of the goniometer. For diffraction case,
:math:`k_I = k_F =k`. For direct geometry inelastic :math:`k_I` is fixed in 
a particular experiment, while for indirect geometry inelastic :math:`k_F` is fixed for
the detector. From equation :eq:`MasterEquation` one can see that the trajectories in the reciprocal
space are simply straight lines, parametrized by :math:`k` for diffraction, 
:math:`k_I` for direct geometry, or :math:`k_F` for indirect geometry.
If we calculate what the :math:`H, K, L` coordinates are for two points, say at 
:math:`k_{min}` and :math:`k_{max}`, we can then write:

.. math::
    :label: proportionality
    
    \frac{H-H_{min}}{H_{max}-H_{min}}=\frac{K-K_{min}}{K_{max}-K_{min}}=
    \frac{L-L_{min}}{L_{max}-L_{min}}=\frac{k-k_{min}}{k_{max}-k_{min}}

Thus, if we know for example that we want to calculate the intersection of
the trajectory with a plane at :math:`H=H_i`, we can just plug in :math:`H_i`
in the above equation and get the corresponding :math:`K_i, L_i, k_i`.

Any trajectory can miss a particular box in :math:`HKL` space, can be along one of the faces
(say if :math:`H_{max}=H_{min}` then all :math:`H` points have the same value), 
or it can intersect the box in exactly two points. If we know the momentum
corresponding to the intersections, say :math:`k_1` and :math:`k_2`, all we need is to 
integrate the incident flux between these two values, and then multiply with the solid 
angle of the detector, in order to obtain the statistical weight of this detector's 
contribution to this particular region in the :math:`HKL` space.

A similar equation to :eq:`proportionality` can be obtained for inelastic scattering, by replacing
:math:`k` with :math:`k_F` for direct geometry or with :math:`k_I` for the
indirect case. We can then relate :math:`k_I` or :math:`k_F` with the enrgy
transfer :math:`\Delta E`, to get the intersections along the energy transfer
direction.

It is important to note that even if we calculate the intersections
of the trajectory with a particular box the norm might still be zero,
since we could have no incident neutron flux corresponding to that 
box in :math:`HKL` space.

.. important:

   Always keep track of the regions you exclude from the measurement
   (masking, cropping original data) when calculating normalization.
   
The way to account for excluded data is algorithm dependent. See
the documentation for each particular implementation.

Symmetrization
++++++++++++++

To improve statistics in a certain region, one can use data from 
different regions of the reciprocal space that are related by the
symmetry of the physics in the material that is being studied.
A simple way to correctly estimate the statistical weight of the
symmetrized data is to apply the symmetry operation on the detector 
trajectories (apply to :math:`H, K, L` 
at :math:`k_{min}` and :math:`k_{max}`) and recalculate the normalization. 
   
Current implementation
++++++++++++++++++++++
As of release 3.3, the normalization can be calculated for single crystal 
diffraction (:ref:`MDNormSCD <algm-MDNormSCD>`) 
and single crystal direct geometry inelastic scattering 
(:ref:`MDNormDirectSC <algm-MDNormDirectSC>`).    
