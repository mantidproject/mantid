.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Sets the neutrons information in the sample. You can either enter
details about the chemical formula or atomic number, or you can provide
specific values for the attenuation and scattering cross sections and
the sample number density. If you decide to provide specific values you
must give values for all three (attenuation and scattering cross
sections and the sample number density), and any formula information
will be ignored. If you miss any of the three specific values then the
other will be ignored.

Neutron scattering lengths and cross sections of the elements and their
isotopes have been taken from
`NIST <http://www.ncnr.nist.gov/resources/n-lengths/list.html>`__.

Chemical Composition with Examples
##################################
- ``H2 O``
- ``(H2)2 O``
- ``D2 O``

Enter a composition as a molecular formula of elements or isotopes. 
For example, basic elements might be ``H``, ``Fe`` or ``Si``, etc. 
A molecular formula of elements might be ``H4-N2-C3``, which 
corresponds to a molecule with 4 Hydrogen atoms, 2 Nitrogen atoms and 
3 Carbon atoms.  Each element in a molecular formula is followed by 
the number of the atoms for that element, specified **without a hyphen**, 
because each element is separated from other elements using a hyphen.
The number of atoms can be integer or float, but must start with a 
digit, e.g. 0.6 is fine but .6 is not. Isotopes may also be included 
in a material composition, and can be specified alone (as in ``Li7``), 
or in a molecular formula (as in ``(Li7)2-C-H4-N-Cl6``).  Note, however, 
that No Spaces or Hyphens are allowed in an isotope symbol specification.
Also Note that for isotopes specified in a molecular expression, the 
isotope must be enclosed by parenthesis, except for two special cases, 
``D`` and ``T``, which stand for ``H2`` and ``H3``, respectively.

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

.. math:: \rho_n = \frac{ZParameter}{UnitCellVolume}

It can can be generated in one of two ways:

1. Specifying it directly with ``SampleNumberDensity``
2. Specifying the ``ZParameter`` and the ``UnitCellVolume`` (or letting
   the algorithm calculate it from the OrientedLattice on the 
   ``InputWorkspace``).

.. categories::
