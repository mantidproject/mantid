.. _Materials:

Materials
=========

.. contents::

Neutron scattering lengths and cross sections of the elements and their
isotopes have been taken from
`NIST <http://www.ncnr.nist.gov/resources/n-lengths/list.html>`__.

Chemical Composition with Examples
##################################
- ``H2 O`` - Isotopically averaged Hydrogen
- ``(H2)2 O`` - Heavy water
- ``D2 O`` - Another way to specify heavy water

Enter a composition as a molecular formula of elements or isotopes.
For example, basic elements might be ``H``, ``Fe`` or ``Si``, etc.
A molecular formula of elements might be ``H4-N2-C3``, which
corresponds to a molecule with 4 Hydrogen atoms, 2 Nitrogen atoms and
3 Carbon atoms.  Each element in a molecular formula is followed by
the number of the atoms for that element, specified **without a hyphen**,
because each element is separated from other elements using a hyphen.

The number of atoms can be integer or float, but must start with a
digit, e.g. 0.6 is fine but .6 is not. This can be used to set elemental ratios
within a chemical composition. For example 95.1% Vanadium 4.9% Niobium can be
expressed as ``V0.951 Nb0.049``. *Warning: Using this representation will
calculate all properties except for SampleNumberDensity which must be
set manually if required*

Isotopes may also be included in a :py:obj:`material
<mantid.kernel.Material>` composition, and can be specified alone (as
in ``(Li7)``), or in a molecular formula (as in ``(Li7)2-C-H4-N-Cl6``).
Note, however, that No Spaces or Hyphens are allowed in an isotope
symbol specification. Also Note that for isotopes specified in a
molecular expression, the isotope must be enclosed by parenthesis,
except for two special cases, ``D`` and ``T``, which stand for ``H2``
and ``H3``, respectively.

Cross Section Calculations
##########################

Each of the cross sections (:math:`\sigma`) are calculated according to

.. math:: \sigma = \frac{1}{N_{atoms}}\sum_{i}\sigma_{i}n_{i}

where :math:`N_{atoms} = \sum_{i}n_{i}`. A concrete example for the total
cross section of ``D2 O``

.. math:: \sigma = \frac{1}{2+1}\left( 7.64*2 + 4.232*1\right) = 6.504\ barns

Number Density
##############

The number density is defined as

.. math:: \rho_n = \frac{N_{atoms}ZParameter}{UnitCellVolume}

It can can be generated in one of three ways:

1. Specifying it directly with ``SampleNumberDensity``.
2. Specifying the ``ZParameter`` and the ``UnitCellVolume`` (or letting
   the algorithm calculate it from the OrientedLattice on the
   ``InputWorkspace``).
3. Specifying the mass density. In this case the number density is calculated as

.. math:: \rho_n = \frac{N_{atoms} \rho_m N_A}{M_r}

where :math:`\rho_m` is the mass density, :math:`N_A` is the Avogadro constant, and :math:`M_r` the relative molecular mass.

Linear Absorption Coefficients
##############################

.. math:: \mu_s = \rho_n \frac{1}{N_{atoms}}\sum_{i}s_{i}n_{i} \text{ units of 1/cm}
.. math:: s = \sigma_{total scattering}
.. math:: \mu_a = \rho_n \frac{1}{N_{atoms}}\sum_{i}a_{i}n_{i} \text{ units of 1/cm}
.. math:: a = \sigma_{absorption} (\lambda=1.8)

A detailed version of this is found in [2].

Normalized Laue
###############

The low-:math:`Q` limit of :math:`S(Q)` is :math:`-L` where :math:`L` is called the normalized Laue term

.. math:: bAverage = <b_{coh}> = \frac{1}{N_{atoms}}\sum_{i}b_{coh,i}
.. math:: bSquaredAverage = <b_{tot}^2> = \frac{1}{N_{atoms}}\sum_{i}b_{tot,i}^2
.. math:: L = \frac{<b_{tot}^2>-<b_{coh}>^2}{<b_{coh}>^2}

References
----------

The data used in this algorithm comes from the following paper.

#. Varley F. Sears, *Neutron scattering lengths and cross sections*, Neutron News **3:3** (1992) 26
   `doi: 10.1080/10448639208218770 <http://dx.doi.org/10.1080/10448639208218770>`_
#. J. A. K. Howard, O. Johnson, A. J. Schultz and A. M. Stringer, *Determination of the neutron
   absorption cross section for hydrogen as a function of wavelength with a pulsed neutron
   source*, J. Appl. Cryst. (1987). 20, 120-122
   `doi: 10.1107/S0021889887087028 <http://dx.doi.org/10.1107/S0021889887087028>`_

.. categories:: Concepts
