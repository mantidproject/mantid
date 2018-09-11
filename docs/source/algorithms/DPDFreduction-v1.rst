.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Reduction algorithm for powder or isotropic data taken at the SNS/ARCS beamline.
Its purpose is to yield a :math:`S(Q,E)` structure factor from which a dynamic pair
distribution function :math:`G(r,E)` can be obtained via the
:ref:`Dynamic PDF <Dynamic PDF>` interface.

Detailed Parameters description
===============================

- **Vanadium**: a preprocessed white-beam vanadium file, meaning that
  all events within a particular wide wavelength range have been rebinned
  into a single histogram, this for every detector.

- **EnergyBins**: user can input a triad :math:`[E_{min}, E_{bin}, E_{max}]`
  or only the energy binning :math:`E_{bin}`. If this is the case,
  :math:`E_{min}` and :math:`E_{max}` are estimated: :math:`E_{min}=-0.5E_i`
  and :math:`E_{max} = 0.95E_i` where :math:`E_i` is the incident energy.

- **MomentumTransferBins**: user has three options. User can input a triad
  :math:`[Q_{min}, Q_{bin}, Q_{max}]`, just :math:`Q_{bin}`,
  or leave this parameter empty. If left empty, :math:`Q_{bin}`
  is estimated as the momentum gained when the neutron gained an
  amount of energy equal to :math:`E_{bin}`. If only :math:`Q_{bin}` is at hand,
  :math:`Q_{min}` and :math:`Q_{max}`
  are estimated with algorithm
  :ref:`ConvertToMDMinMaxLocal <algm-ConvertToMDMinMaxLocal>`.

- **NormalizeSlices**: This option will normalize the final :math:`S(Q,E)`
  for each energy bin, independent of each other.

- **CleanWorkspaces**: a series of intermediate workspaces are generated during the
  reduction process. If this option is set to False, the user may want to inspect these
  (listed in the order they are produced):

  + `vanadium`: vanadium events workspace
  + `reduced`: sample runs after DgsReduction algorithm
  + `ec_reduced`: empty can after :ref:`DgsReduction <algm-DgsReduction>` algorithm
  + `vanadium_S_theta`: vanadium after :ref:`GroupDetectors <algm-GroupDetectors>` algorithm.
  + `ec_S_theta_E`: reduced empty can after :ref:`GroupDetectors <algm-GroupDetectors>`
    algorithm.
  + `S_theta_E`: reduced sample runs after :ref:`GroupDetectors <algm-GroupDetectors>`
    algorithm and after subtraction of empty can `ec_S_theta_E`.
  + `S_theta_E_normalized`: `S_theta_E` divided by vanadium vanadium_S_theta
  + `S_theta_E_normalized_interp`: `S_theta_E_normalized` with interpolation over the
    detector gaps.
  + `S_Q_E`: `S_theta_E_normalized_interp` converted to an events MDWorkspace
    with momentum transfer instead of angles.
  + `S_Q_E_binned`: Histogram MDWorkspace after application of
    :ref:`BinMD <algm-BinMD>` algorithm to `S_Q_E`.
  + `S_Q_E_sliced`: conversion of `S_Q_E_binned` to a MatrixWorkspace with
    :ref:`ConvertMDHistoToMatrixWorkspace <algm-ConvertMDHistoToMatrixWorkspace>` algorithm.
  + `S_Q_E_sliced_norm`: `S_Q_E_sliced` where each E-bin has been normalized.

- **OutputWorkspace**: The final :math:`S(Q,E)` which can serve as
  input for the :ref:`Dynamic PDF <Dynamic PDF>` interface.

Interpolation
=============

The ARCS instrument has two gaps at particular :math:`\theta` angles due to arrangement
of the banks

.. figure:: /images/DPDFreduction_fig1.png
   :scale: 50 %
   :align: center

The gaps lead to empty bins in the :math:`S(\theta,E)` histogram which in turn generate
significant errors in the final :math:`S(Q,E)` for certain values of :math:`Q`.
To prevent this we carry out a linear interpolation in :math:`S(\theta,E)`
at the blind-strip :math:`\theta` angles.

Normalization by number of events
=================================
If user desires to plot the OutputWorkspace with Mantid's slice viewer, user
should choose the "# Events Normalization" view. The last step in the reduction
is performed by executing
:ref:`ConvertMDHistoToMatrixWorkspace <algm-ConvertMDHistoToMatrixWorkspace>`,
which requires *NumEventsNormalization*. Our input workspace has as many spectra
as instrument detectors. Each detector has a 2D binning in
:math:`Q` and :math:`E`.
Each detector is at a particular :math:`\theta` angle, thus
:math:`E` and :math:`Q` are related by:

:math:`E(Q) \rightarrow \frac{\hbar Q^2}{2m} =  2E_i + E -2\sqrt{(E_i+E)E_i} \ \ \cos\theta`

That means that only :math:`(Q,E)` bins satisfying the above condition have counts.
Thus for detector :math:`i` we have number of counts
:math:`N_i(Q_j,E_k) \neq 0` if the :math:`(Q_j, E_k)` pair satisfy
the above condition. This represents a trajectory in :math:`Q-E` space.

When we execute
:ref:`ConvertMDHistoToMatrixWorkspace <algm-ConvertMDHistoToMatrixWorkspace>`
with :math:`Q` binning :math:`\Delta Q` and E binning :math:`\Delta E`,
we go detector by detectory and we look at the fragment of the
:math:`Q(E)` trajectory enclosed in the cell of Q-E phase space
denoted by the corners :math:`(Q,E)`, :math:`(Q+\Delta Q,E)`,
:math:`(Q,E+\Delta E)` and :math:`(Q+\Delta Q,E+\Delta E)`.
Thus we have for detector :math:`i` to look at the :math:`(Q_j, E_k)` pairs
within this cell for detector :math:`i`, with associated
:math:`N_i(Q_j,E_k)` counts and associated scattering cross-section:

:math:`(\frac{d\sigma^2}{dE d\Omega})_{i,j,k} \ \ (Q_j,E_k) = \frac{N_i(Q_j,E_k)}{d\Omega \delta E}`

The scattering cross-section in the aforementioned cell of dimensions
:math:`\Delta Q` x :math:`\Delta E` is the *average* of all
the scattering cross sections:

:math:`\frac{d\sigma^2}{\Delta E d\Omega}(Q,E) = \sum\limits_{i,j,k}(\frac{d\sigma^2}{\delta E d\Omega})_{i,j,k} \ \ (Q_j,E_k) \cdot \Pi_{Q,Q+\Delta Q} \ \ \ (Q_j) \cdot \Pi_{E,E+\Delta E} \ \ \ (E_k) / \sum\limits_{i,j,k} \Pi_{Q,Q+\Delta Q} \ \ \ (Q_j) \cdot \Pi_{E,E+\Delta E} \ \ \ (E_k)`

where :math:`\Pi_{a,b} \ (x)` is the
`boxcar function <http://mathworld.wolfram.com/BoxcarFunction.html>`_

.. categories::

.. sourcelink::
