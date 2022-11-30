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

It can can be generated in one of several ways:

1. Specifying it directly with ``NumberDensity``.
2. Specifying the ``ZParameter`` and the ``UnitCellVolume`` (or letting
   the algorithm calculate it from the OrientedLattice on the
   ``InputWorkspace``).
3. If a chemical formula consisting of a single element has been supplied the number density will be looked up from Mantid tables
4. The default behaviour is to deduce it from an effective number density (see below) and an optional packing fraction supplied as ``PackingFraction`` (which is assumed to be 1 if not supplied)

The effective number density is defined as

.. math:: \rho_{n,eff} = f \rho_n

where :math:`f` is the packing fraction

It can be specified in one of several ways:

1. Specifiying it directly with ``EffectiveNumberDensity``
2. Specifying the mass density. In this case the effective number density is calculated as follows:

.. math:: \rho_{n,eff} = \frac{N_{atoms} \rho_m N_A}{\sum_{i}n_{i}M_i}

where :math:`\rho_m` is the mass density, :math:`N_A` is the Avogadro constant, and :math:`M_i` is the relative molecular mass of the ith atom.

3. The default behaviour is to set it equal to the full number density multipled by the optional packing fraction (which is assumed to be 1 if not supplied)


The effective number density, :math:`\rho_{n,eff}`, should be used for :ref:`absorption calculations<Sample Corrections>`.
However, the number density, :math:`\rho_n` should be used for refinements and converting between real space representations.

If both a number density and effective number density are supplied using the non-default behaviours then a packing fraction will be calculated from their ratio.

Attenuation Coefficients
##############################

The attenuation effect is calculated according to the following formula:

.. math:: \exp(-\rho_n(\mu_s+\mu_a)t)

where :math:`\rho_n` is in unit of :math:`\AA^{-3}`, :math:`t` is the material thickness in cm, the two attenuation coefficients representing scattering and absorption (:math:`\mu_s` and :math:`\mu_a` respectively) are calculated as follows:

.. math:: \mu_s = \rho_n \frac{1}{N_{atoms}}\sum_{i}s_{i}n_{i} \text{ units of 1/cm}
.. math:: s = \sigma_{total scattering}
.. math:: \mu_a = \rho_n \frac{1}{N_{atoms}}\sum_{i}a_{i}n_{i} \text{ units of 1/cm}
.. math:: a = \frac{\lambda}{\lambda_0} \sigma_{absorption} (\lambda_0) \text{ where } \lambda_0=1.8\AA

A detailed version of this is found in [2].

The sum of the two attenuation coefficients can be replaced by an externally measured profile of attenuation versus wavelength if the scattering effect is wavelength dependent eg if a material is crystalline and shows some Bragg edges in its attenuation profile.
Mantid supports a space delimited text file format for the externally measured profile containing the following columns:

- wavelength (in :math:`\AA`)
- attenuation factor (in :math:`mm^{-1}`)
- error (currently ignored)

The Xray Attenuation coefficients can also be provided by text file with the following columns containing:

- energy (in :math:`KeV`)
- attenuation factor (in :math:`mm^{-1}`)
- error (currently ignored)

Any lines not following this format (eg header rows) are ignored. The file must have a .DAT file extension.

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
