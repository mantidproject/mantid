.. _DynamicalStructureFactorFromAbInitio:

Ab initio calculation of dynamical structure factor (S)
=======================================================


Introduction
------------

The purpose of this document is to explain the link between theoretical and experimental :math:`S(\mathbf{Q}, \omega)` and to
describe in general how the theoretical :math:`S` is calculated from from *ab initio* data by plugins in Mantid.

During an inelastic neutron scattering experiment, a sample is exposed to neutron flux and a response is recorded in the form of dynamical structure factor, :math:`S(\mathbf{Q}, \omega)`.
In principle, one obtains a vibrational spectrum that can be quite difficult to analyse; in crystalline materials this is
related to the wavevector-dependent *phonon* spectrum.
In order to better understand experimental outputs, one can compare with results from modelling.
*Ab initio* calculations, especially within density-functional theory (DFT) [#Kohn1964]_, have proven quite successful in predicting vibrational spectra.

.. image:: ../images/dft_phonon_scheme.png
    :align: center

The usual workfow for calculating phonon spectra within DFT is presented in the figure above. First, one defines an
initial guess for the structure of interest.
The initial guess should be as close as possible to an experimental structure, and is usually derived from elastic X-ray and/or neutron scattering measurements.
Then the structure parameters are locally optimised within DFT, finding the nearest structure that minimises the DFT energy.
At this point, there should be no net force on the atoms.
For this "relaxed" structure the dynamical matrix is calculated, either by finite displacements or perturbation theory.
The dynamical matrix is related to the Hessian (the second derivative of the system Hamiltonian with respect to atomic displacements) by a Fourier transform:
the eigenvectors obtained from diagonalisation of this matrix are atomic displacements
and the eigenvalues are the squared frequencies of the corresponding movements.
These vibrational *modes* are related to the *fundamental* vibrational excitations of the system;
using this displacement and frequency information one can calculate theoretical :math:`S(\mathbf{Q}, \omega)`.
In Abins this is calculated for each atom separately,
then the total spectrum is obtained as a sum over all atomic contributions.


Working equations
-----------------

Powder
~~~~~~

.. image:: ../images/s_powder_scheme.png
    :align: center

In DFT studies of solid materials, the simulation region is generally a finite unit cell with periodic boundary conditions.
This models an infinite perfect crystal;
in order to compare such calculations with powder experiments, orientational averaging should be considered.
Usually a semi-empirical model is applied [#Howard1983]_:sup:`,` [#Howard1983b]_:

:math:`S^j (\mathbf{Q},\omega_i) = S^j (Q,\omega_i) = \frac{Q^2 \mathrm{Tr}B_{\omega_i}}{3} exp\left(-Q^2 \alpha^j_{\omega_i} coth^2\left(\frac{\hbar \omega_i}{2 k_B T}\right)  \right)\sigma^j`

where :math:`B` and :math:`A` are tensors created from atomic displacements in the following way:

:math:`B^j_{\omega_i} = c^j_{\omega_i}(c^{j}_{\omega_i})^T M^j  \frac{C_1}{\omega_i}`

:math:`A^j = \sum_i B^j_{\omega_i}`

with

:math:`Q` -- momentum transfer due to neutron scattering.  The momentum transfer :math:`\mathbf{Q}` is a technically a vector. However, the powder averaging of :math: `S` allows it to be represented as a scalar.

:math:`\alpha^j_{\omega_i}` -- semi-empirical parameter calculated as: :math:`\alpha^j_{\omega_i} = \frac{1}{5} \left \lbrace \mathrm{Tr} A^j  + \frac{2 B^j_{\omega_i}: A^j}{\mathrm{Tr} B^j_{\omega_i}} \right\rbrace`

:math:`\mathrm{Tr}` -- trace operation

:math:`:` --  tensor contraction operation

:math:`j` -- indicates :math:`j`-th atoms

:math:`i` -- indicates :math:`i`-th energy transition

:math:`\omega_i` -- frequency for :math:`i`-th transition in :math:`cm^{-1}` (called also mode or fundamental)

:math:`c^j_{\omega_i}`  -- atomic displacement for :math:`j`-th atom and :math:`i`-th frequency in atomic units

:math:`M_j` -- mass of :math:`j`-th atom in atomic units

:math:`C_1` --  :math:`\hbar / (4 / \pi)` expressed in spectroscopic units

:math:`k_B` -- Boltzmann constant

:math:`T` -- temperature in K

:math:`\sigma_j` -- cross-section for :math:`j`-th atom


The formula above covers the *first-order quantum events* -- specifically the transitions :math:`0 \rightarrow 1` for each phonon.
The :math:`1 \rightarrow 0` events (i.e. energy *to* the scattered neutron) would be infrequent at experimental conditions and are neglected.
In order to reconstruct the full spectrum one has to also consider higher-order quantum events.
For second-order quantum events one should not only
consider transitions :math:`0 \rightarrow 2`, but also simultaneous transitions :math:`0 \rightarrow 1`, :math:`0 \rightarrow 1'` for different phonons.
Within the harmonic approximation all second-order transitions form the following set: :math:`\lbrace \omega_1 +
\omega_1, \omega_1 + \omega_2, \omega_1 + \omega_3, \ldots, \omega_p + \omega_p \rbrace`.
The cardinality of this set is :math:`p^2`, where :math:`p` is a number of fundamentals.
In practice one can reduce this number by taking into consideration a realistic energy window
and neglecting those :math:`\omega_{ij}=\omega_i + \omega_j` for which :math:`S(Q, \omega_i)` or :math:`S(Q, \omega_j)` is negligible.
Within the harmonic approximation each phonon is treated as independent harmonic quantum oscillator.  The formula for :math:`S(Q, \omega_{ik})` is as follows [#Mitchell]_:

:math:`S^j(Q, \omega_{ik}) = \frac{Q^4}{15  C}\left( \mathrm{Tr}B^j_{\omega_i}\mathrm{Tr}B^j_{\omega_k} + B^j_{\omega_i}:B^j_{\omega_k} + B^j_{\omega_k}:B^j_{\omega_i} \right) exp\left(-Q^2 \beta^j coth^2\left(\frac{\hbar \omega_{ik}}{2 k_B T} \right) \right)\sigma^j`

where

:math:`\beta^j = A^j / 3`.

:math:`C` is a constant: if :math:`i=j` then :math:`C=2`, otherwise :math:`C=1`.

Similarly, one can define the contribution for the third quantum order events (:math:`0 \rightarrow 3`, simultaneous  :math:`0 \rightarrow 1`, :math:`0 \rightarrow 1'`, :math:`0 \rightarrow 1''` for different oscillators, etc.) [#Mitchell]_:

:math:`S^j(Q, \omega_{ikl}) = \frac{9Q^6}{543}\left( \mathrm{Tr}B^j_{\omega_i} \mathrm{Tr}B^j_{\omega_k} \mathrm{Tr}B^j_{\omega_l}  \right)  exp\left(-Q^2 \beta^j coth^2\left(\frac{\hbar \omega_{ikl}}{2 k_B T}\right) \right)\sigma^j`.

Usually in order to reconstruct the experimental spectrum it is sufficient to include contributions up to the fourth order (:math:`0 \rightarrow 4` , simultaneous :math:`0 \rightarrow 1`, :math:`0 \rightarrow 1'`, :math:`0 \rightarrow 1''`, :math:`0 \rightarrow 1'''` for different oscillators, etc.)

:math:`S^j(Q, \omega_{iklm}) = \frac{27Q^8}{9850}\left( \mathrm{Tr}B^j_{\omega_i} \mathrm{Tr}B^j_{\omega_k} \mathrm{Tr}B^j_{\omega_l}\mathrm{Tr}B^j_{\omega_m}  \right) exp\left(-Q^2 \beta^j coth^2\left(\frac{\hbar \omega_{iklm}}{2 k_B T}\right) \right)\sigma^j`. [#Mitchell]_

In the same way as for the second quantum order events, one can reduce the number of energy transitions by imposing the appropriate energy window and neglecting small :math:`S`.

After evaluating the above equations one obtains the discrete :math:`S` for each quantum order and for each atom: :math:`S_\mathrm{discrete}`.
In order to compare these functions with an experimental spectrum one has to convolve them with experimental resolution

:math:`S_\mathrm{theory}^{nj}(Q, \omega) = S_\mathrm{discrete}^{nj}(Q, \omega) * f(\omega)`

where:

:math:`j` -- indicates :math:`j`-th atoms

:math:`n` -- indicates :math:`n`-order event

:math:`f(\omega)` -- is a resolution function

:math:`S_\mathrm{theory}` -- is *theoretical* :math:`S` to be compared with experimental results.

For `TOSCA <http://www.isis.stfc.ac.uk/instruments/tosca/tosca4715.html>`_  and TOSCA-like instruments :math:`f(\omega)` has the following form:

:math:`f(\omega)=1.0 / \sqrt{\sigma(\omega)  \pi}  \exp(-(\omega)^2  / \sigma(\omega))`

where:

:math:`\sigma(\omega) = A  \omega^2  + B  \omega + C`

with :math:`A`, :math:`B`, :math:`C` as constants.

Moreover, in case of TOSCA and TOSCA-like instruments, the length of momentum transfer depends on frequency (*indirect geometry spectrometer*).
The formula for :math:`Q^2` is as follows:

:math:`Q^2(\omega)=k^2_i(\omega) + k^2_f - 2  \sqrt{k^2_i(\omega)  k^2_f} cos(\theta)`

where:

:math:`k^2_i(\omega)=(\omega + E_{final})  \hbar/ (4  \pi)` expressed in the spectroscopic units

:math:`k^2_f=E_{final}  \hbar/(4 \pi)`

with

:math:`E_{final}` -- being the final energy on the crystal analyser in :math:`cm^{-1}` and

:math:`\theta` -- is the crystal analyser angle in radians. (TOSCA has two angles to consider, corresponding to the forward- and back-scattering detectors).

Current implementation
----------------------

Calculation of theoretical :math:`S` from *ab initio* results is implemented in :ref:`Abins <algm-Abins>`. At the moment Abins supports phonon outputs from the
`CASTEP <http://www.castep.org/>`_, `CRYSTAL <http://www.crystal.unito.it/index.php>`_, Gaussian and DMOL3 *ab initio* codes.
The Gamma-point frequencies are used and phonon bands are assumed to be flat throughout the Brillouin zone; this assumption is primarily applicable for incoherent scattering in molecular crystals.
Instrument parameters are included for
`TOSCA <http://www.isis.stfc.ac.uk/instruments/tosca/tosca4715.html>`_ and should be useful for TOSCA-like instruments.

Citing Abins
------------

If Abins is used as part of your data analysis routines, please cite the relevant reference [#Dymkowski2018]_.

References
----------

.. [#Kohn1964] W. Kohn et al., *Inhomogeneous Electron Gas*, Phys. Rev. B {\bf 136}, 864 (1964).

.. [#Howard1983] J. Howard, B.C. Boland, J. Tomkinson, *Intensities in inelastic neutron scattering spectra: a test of recent theory*, Chem. Phys. 77 (1983).

.. [#Howard1983b] J. Howard and J. Tomkinson, *An analytical method for the calculation of the relative intensities of bending and stretching modes in inelastic neutron scattering spectra*, Chem. Phys. Letters 98 (1983).

.. [#Mitchell] P. C H Mitchell, S. F. Parker, A. J. Ramirez-Cuesta, J. Tomkinson, *Vibrational Spectroscopy with Neutrons With Applications in Chemistry, Biology, Materials Science and Catalysis*, ISBN: 978-981-256-013-1

.. [#Dymkowski2018] K. Dymkowski, S. F. Parker, F. Fernandez-Alonso and S. Mukhopadhyay,  “AbINS: The modern software for INS interpretation” , Physica B, doi:10.1016/j.physb.2018.02.034 (2018).

.. categories:: Concepts
