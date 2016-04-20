.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Computes the phonon density of states from an inelastic neutron 
scattering measurement of a powder or polycrystalline sample,
assuming that all scattering is incoherent, using the formula 
(Thermodynamic Properties of Solids, eds. Chaplot, Mittal, Choudhury,
Chapter 3) for the 1-phonon incoherent scattering function:

:math:`S^{(1)}_{\mathrm{inc}}(Q,E) = 
   \exp\left(-2\bar{W}(Q)\right) \frac{Q^2}{E} 
   \langle n+\frac{1}{2}\pm\frac{1}{2} \rangle
   \left[ \sum_k \frac{\sigma_k^{\mathrm{scatt}}{2m_k} g_k(E) \right]`

where the term in square brackets is the neutron weighted densiy of 
states which is calculated by this algorithm, and :math:`g_k(E)` is
the partial density of states for each component (element or isotope)
:math:`k` in the material. :math:`m_k` is the relative atomic mass of the
component.

The average Debye-Waller factor :math:`\exp\left(-2\bar{W}(Q)\right)` is
calculated using an average mean-square displacement :math:`\langle u \rangle`,
using :math:`W=Q^2\langle u\rangle/2`. 

If the data has been normalised to a Vanadium standard measurement, the
output of this algorithm is the neutron weighted density of states in
milibarns/steradians per formula unit per meV (or per cm^-1). If the sample
material has been set and is found to be a pure element, then an additional
option will be enabled to calculate the DOS in states per meV (states per 
cm^-1) by dividing by the scattering cross-section and multiplying by the
relative atomic mass.

Restrictions on the Input Workspace
###################################

The input workspace must have units of Momentum Transfer and
contain histogram data with common binning on all spectra.

Usage
-----

.. include:: ../usagedata-note.txt

**ISIS Example**

.. testcode:: ExIsis

    # The MARIReduction_Sample file can be found in the User scripts repository
    # under direct_inelastic/mari
    import MARIReduction_Sample
    # Data is from a measurement of Aluminium pellets on Mari during a neutron training course
    MARIReduction_Sample.iliad_mari(20228,60,19717,20383,25,27)
    MAR20228_DOS = ComputeIncoherentDOS('MAR20228_SQW',T=300,MeanSquareDisplacement=0.01)

Output

.. testoutput:: ExIsis

.. categories:: Algorithms Inelastic

