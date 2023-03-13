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

For this "relaxed" structure the dynamical matrix is calculated at a set of phonon wavevectors :math:`\mathbf{k}`, and converted to a set of physical frequencies and displacements that contribute to the dynamics of the material.
The dynamical matrix is related by a Fourier transform (at the given :math:`\mathbf{k}`-points) to the Hessian of the system -- the second derivative of the system Hamiltonian with respect to atomic displacements.
While it is possible to calculate the dynamical matrix at arbitrary :math:`\mathbf{k}`-points by perturbation theory,
it is typical to compute these on a regular grid or, equivalently, compute the Hessian up to some real-space supercell of the crystal structure.
This model allows an inexpensive "Fourier interpolation" of frequencies and displacements at arbitrary :math:`\mathbf{k}`-points, as long as the supercell size is sufficient to contain the relevant atomic interactions.

Solution of the dynamical matrix vibrational at each sampled wavevector :math:`\mathbf{k}` produces a set of *modes*, which are related to the *fundamental* vibrational excitations of the system. :math:`\mathbf{k}` corresponds to a neutron momentum transfer :math:`\mathbf{Q}`,
with eigenvalues related to the energy transfer :math:`\omega` and eigenvectors related to atomic displacements.
By inserting this second-order model into the response-function theory of neutron scattering we can calculate :math:`S(\mathbf{Q}, \omega)` within the harmonic approximation.

Some further simplifications are made in order to account for powder-averaging and higher-order excitations at reasonable computational cost: the method current implemented in Abins is a

- DOS-like
- almost-isotropic
- incoherent approximation
- with simplified high-order terms.

The incoherent approximation allows each atom to be calculated separately,
then the total spectrum is obtained as a sum over all atomic contributions.

This method is well-established for molecular spectroscopy; studies of inorganic crystals, on the other hand, tend to calculate the coherent scattering and neglect multi-phonon terms.

Working equations
-----------------

.. image:: ../images/s_powder_scheme.png
    :align: center

The vibrational part of the structure factor :math:`S(\mathbf{Q}, \omega)` is related by a Fourier transform to the intermediate scattering function :math:`F(\mathbf{Q}, t) = \frac{1}{N} \sum_{i,i^\prime} \left< \exp[i \mathbf{Q} (\mathbf{r}_i(t) - \mathbf{r}_i^\prime(0))] \right>`. This is a double-sum over atom pairs :math:`i, i^\prime` in a thermally-averaged correlation function.

Solving this for a quantum harmonic oscillator, making the incoherent approximation and a Taylor expansion of the exponential term allows the double-sum over atoms to be separated into a sum over atoms :math:`i`, vibrational mode indices :math:`\nu` and quantum orders :math:`n`. These intensities may be computed independently:

:math:`S_i(\mathbf{Q}, n\omega_{\nu}) = \sigma_i \frac{[\mathbf{Q Q : B}_{\nu,i} \left<n + 1\right>]^{n}}{n!} \exp(-\mathbf{Q Q : A}_i)`

in which :math:`\sigma_i` is an atomic cross-section while :math:`\mathbf{B_{\nu,i}}` and :math:`\mathbf{A}_i` are :math:`3 \times 3` quadratic dispacement tensors of individual phonon modes and the overall atomic dispacements, respectively.
In Einstein notation the contraction of two 2--D tensors :math:`\mathbf{A}:\mathbf{B}` is :math:`\mathbf{A}_{ij}\mathbf{B}_{ij}`.

Similar expressions are formed for combination modes in which :math:`n\omega` becomes :math:`\sum^n \omega_\nu` and :math:`\mathbf{B}_{\nu,i}^{n}` is replaced by :math:`\prod_\nu^n \mathbf{B}_{\nu,i}`.

The displacement tensors can be obtained from the calculated phonon eigenvectors :math:`{\mathbf{c}}`: :math:`\mathbf{B_{\nu_i}}` is mode-dependent and thermally-occupied in the expression above by :math:`\left<n+1\right>` Bose statistics while :math:`\mathbf{A}_i` is a property of each atom site and includes occupation :math:`\left<2n+1\right>`: at low temperature both terms reduce to 1.

:math:`\mathbf{B}_{\nu,i} = \mathbf{c}_{\nu,i} \mathbf{c}_{\nu,i}^\intercal \frac{\hbar}{2 M_i \omega_\nu}`

:math:`A_i = \sum_i \mathbf{B}_{\omega,i} \left<2n + 1\right>`

where :math:`\mathbf{c}_{i, \nu}`  -- normalised eigenvector for atom :math:`i` in mode :math:`\nu` and :math:`M_i` is the mass of atom :math:`i`. For the rest of this document, we define :math:`\mathbf{B}` to include the Bose factor :math:`\left<n+1\right>`.

In DFT studies of solid materials, the simulation region is generally a finite unit cell with periodic boundary conditions.
This models an infinite perfect crystal; in order to compare such calculations with powder experiments, orientational averaging should be considered.
In Abins, the "almost-isotropic approximation" is applied to the spherical integration over reciprocal space, replacing the vector :math:`\mathbf{Q}` with a scalar :math:`Q` [#Howard1983]_:sup:`,` [#Howard1983b]_:

:math:`S_i (Q,\omega_\nu) = \sigma_i \frac{Q^2 \mathrm{Tr}\mathbf{B}_{\nu,i}}{3} \exp\left(-Q^2 \alpha_{\nu,i} \right)`

where

:math:`\alpha_{\nu,i} = \frac{1}{5} \left \lbrace \mathrm{Tr} \mathbf{A}_i  + \frac{2 \mathbf{B}^{\omega_\nu,i}: \mathbf{A}_i}{\mathrm{Tr} \mathbf{B}^{\omega_\nu,i}} \right\rbrace`

Where :math:`\mathrm{Tr}` denotes the "trace".
In this case, :math:`\mathbf{A}:\mathbf{B}` becomes equivalent to :math:`\mathrm{Tr}(\mathbf{A}\cdot\mathbf{B})` due to the symmetry of the quadratic displacement tensors.

Note that the exponential term is no longer a "pure" Debye--Waller factor, as some mode-dependence is introduced by the powder-averaging.
We have also conflated the phonon mode indices and their original wavevector :math:`\mathbf{k}` into a single index :math:`\nu`; this is a "DOS-like approximation" in which we assume that the reciprocal lattice spacing is small relative to the observed :math:`Q`, and so spherical averaging will approximate an even sampling of the Brillouin zone.

The formula above covers the *first-order quantum events* -- the transitions :math:`0 \rightarrow 1` for each phonon.
The :math:`1 \rightarrow 0` events (i.e. energy *to* the scattered neutron) are currently neglected, as the contribution vanishes at low temperature.
The powder-averaging derivations are more complex for higher-order quantum events.
At second order some exponential terms are neglected, and isotropic Debye-Waller factor is used without any mode-dependence [#Mitchell]_.

:math:`S_i^{n=2}(Q, \omega_\nu + \omega_{\nu^{\prime}}) = \frac{Q^4}{15  C}\left( \mathrm{Tr}\mathbf{B}_{i,\nu}\mathrm{Tr}\mathbf{B}_{i,\nu^\prime} + \mathbf{B}_{i,\nu}:\mathbf{B}_{i,\nu^\prime} + \mathbf{B}_{i,\nu^\prime}:\mathbf{B}_{i,\nu} \right) \exp\left(-Q^2 \mathbf{A}_i / 3 \right)\sigma_i`

where :math:`C = \begin{cases} 2  & \textrm{if $\nu=\nu^\prime$} \\ 1 & \textrm{otherwise} \end{cases}`

For higher-order events we can further simplify with a fully isotropic approximation :math:`\mathbf{Q Q}:\mathbf{B} \approx Q^2 \mathrm{Tr}\mathbf{B} / 3`:

:math:`S_i^n(Q, \omega_\nu + \omega_{\nu^{\prime}} + \cdots) = \frac{Q^{2n}}{n!} \left[\prod_\nu^n \frac{\mathrm{Tr}\mathbf{B}_{i,\nu}}{3} \right] \exp\left(-Q^2 \frac{\mathrm{Tr}\mathbf{A}_i}{3} \right) \sigma_i`

While a significant simplification, this allows the "combinatorial explosion" of phonon-mode combinations to be avoided. The mode-by-mode terms are combined into an energy spectrum

:math:`S_i^n(Q, \omega) = \sum_{(\nu, \nu^\prime, \cdots)\in \mathrm{fundamentals}} S_i(Q, \omega_\nu + \omega_{\nu^\prime} + \cdots) \delta(\omega - [\omega_\nu + \omega_{\nu^\prime} + \cdots])`

in which we identify a recursive term :math:`s_i`

:math:`S_i^n(Q, \omega) =  \frac{\sigma_i}{n!}s_i^n(Q, \omega) \exp\left(-Q^2 \mathbf{A}_i / 3 \right)`

:math:`s_i^n(Q, \omega) = Q^{2n} \sum_{(\nu, \nu^\prime, \cdots)\in \mathrm{fundamentals}} \left[\prod_{\nu = \nu, \nu^\prime, \cdots} \frac{\mathrm{Tr}\mathbf{B}_{i,\nu}}{3} \right] \delta(\omega - [\omega_\nu + \omega_{\nu^\prime} + \cdots])`

:math:`s_i^n(Q, \omega) = \sum_{\nu\in \mathrm{fundamentals}} \frac{Q^2\mathrm{Tr}\mathbf{B}_{i,\nu}}{3} \delta(\omega - \omega_\nu) * s_i^{n-1}(Q, \omega)`

By performing the convolution (:math:`*`) operations on a numerical grid it is possible to reach :math:`n=10` without computing an infeasible :math:`(3N_\mathrm{atoms} N_\mathbf{k})^{10}` intensity values.

Resolution
----------

After evaluating the above equations one obtains the discrete :math:`S` for each quantum order and for each atom: :math:`S_\mathrm{discrete}`.
In order to compare these functions with an experimental spectrum one has to convolve them with experimental resolution :math:`f(\omega)`

:math:`S_{i,\mathrm{theory}}^{n}(Q, \omega) = S_{i,\mathrm{discrete}}^{n}(Q, \omega) * f(\omega)`

For `TOSCA <http://www.isis.stfc.ac.uk/instruments/tosca/tosca4715.html>`_  and TOSCA-like instruments :math:`f(\omega)` is treated as a Gaussian function with energy-dependent width :math:`\sigma(\omega)`:

:math:`f(\omega)=\frac{\exp(-(\omega)^2  / \sigma(\omega))}{\sqrt{\sigma(\omega)  \pi}}`

The application of an energy-dependent resolution function is not trivial. For efficiency Abins uses an approximate scheme :ref:`documented here <AbinsInterpolatedBroadening>`.

Energy-Q relations
------------------

Although we are nominally measuring the property :math:`S(\mathbf{Q}, \omega)` or :math:`S(Q, \omega)`, in time-of-flight neutron spectrometers, :math:`\mathbf{Q}` and :math:`\omega` are not independent. :ref:`Abins <algm-Abins>` calculates 1-D :math:`S(\omega)` spectra in which the corresponding value(s) of :math:`Q` are implicitly determined by the instrument design and settings. :ref:`Abins2D <algm-Abins2D>` models multi-detector instruments that generate a more complete 2-D map but there are still kinematic constraints on the measurement region. The :ref:`QE Coverage` interface can be used to explore and plot these :math:`\omega`--:math:`Q` relations.

Current implementation
----------------------

Calculation of theoretical :math:`S` from *ab initio* results is implemented in :ref:`Abins <algm-Abins>` and :ref:`Abins <algm-Abins2D>`. At the moment Abins supports phonon outputs from the *ab initio* codes
`CASTEP <http://www.castep.org/>`_, `CRYSTAL <http://www.crystal.unito.it/index.php>`_, Gaussian, DMOL3 and VASP, as well as force constants computed with `Phonopy <https://phonopy.github.io/phonopy/index.html>`_.

Due to the "DOS-like approximation" bands are assumed to be flat throughout the Brillouin zone.
While only the incoherent scattering spectrum is calculated, coherent weights may be included to make an "incoherent approximation" to the full spectrum.
The method implemented in Abins is primarily applicable for incoherent scattering in molecular crystals.

Instrument models are included for `TOSCA <http://www.isis.stfc.ac.uk/instruments/tosca/tosca4715.html>`_, `LAGRANGE <https://www.ill.eu/users/instruments/instruments-list/in1-taslagrange/description/instrument-layout>`_ and `PANTHER <https://www.ill.eu/users/instruments/instruments-list/panther/description/instrument-layout>`_ using parameterised fits for the resolution function :math:`\sigma(\omega)`.
Instruments `MAPS <https://www.isis.stfc.ac.uk/Pages/maps.aspx>`_, `MARI <https://www.isis.stfc.ac.uk/Pages/mari.aspx>`_ and `MERLIN <https://www.isis.stfc.ac.uk/Pages/merlin.aspx>`_ use :ref:`PyChop` to obtain values for a polynomial fit.

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
