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

    # This is using a generic reduction routine so that automated tests can run
    # You should use the reductions scripts provided by instrument scientists
    from Direct import DirectEnergyConversion
    from mantid.simpleapi import *
    rd = DirectEnergyConversion.DirectEnergyConversion('MARI')
    # Run #21334 on MARI is a measurement of a large Aluminium sample from the neutron
    # training course.
    ws = rd.convert_to_energy(21334, 21335, 60, [-55,0.05,55], 'mari_res2013.map', 
        monovan_run=21347, sample_mass=106.4, sample_rmm=27, monovan_mapfile='mari_res2013.map')
    ws_sqw = SofQW3(ws, [0,0.1,12], 'Direct', 60)
    SetSampleMaterial(ws_sqw,'Al')
    ws_dos = ComputeIncoherentDOS(ws_sqw, Temperature=5, StatesPerEnergy=True)

Output

.. testoutput:: ExIsis

.. categories:: Algorithms Inelastic

