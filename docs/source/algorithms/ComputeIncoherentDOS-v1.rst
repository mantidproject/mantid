.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Computes the phonon density of states from an inelastic neutron 
scattering measurement of a powder or polycrystalline sample,
assuming that all scattering is incoherent, using the formula 
(Thermodynamic Properties of Solids, eds. Chaplot, Mittal, Choudhury,
Chapter 3) for the 1-phonon incoherent scattering function:

:math:`S^{(1)}_{\mathrm{inc}}(Q,E) = \exp\left(-2\bar{W}(Q)\right) \frac{Q^2}{E} \langle n+\frac{1}{2}\pm\frac{1}{2} \rangle \left[ \sum_k \frac{\sigma_k^{\mathrm{scatt}}}{2m_k} g_k(E) \right]`

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

The following code will run a reduction on a MARI (ISIS) dataset and apply
the algorithm to the reduced data. The datafiles (runs 21334, 21335, 21347) and
map file 'mari_res2013.map' should be in your path. Run number 21335 is a 
measurement of a large Aluminium sample from the neutron training course.

.. code:: python

    from Direct import DirectEnergyConversion
    from mantid.simpleapi import *
    rd = DirectEnergyConversion.DirectEnergyConversion('MARI')
    ws = rd.convert_to_energy(21334, 21335, 60, [-55,0.05,55], 'mari_res2013.map', 
        monovan_run=21347, sample_mass=106.4, sample_rmm=27, monovan_mapfile='mari_res2013.map')
    ws_sqw = SofQW3(ws, [0,0.1,12], 'Direct', 60)
    SetSampleMaterial(ws_sqw,'Al')
    ws_dos = ComputeIncoherentDOS(ws_sqw, Temperature=5, StatesPerEnergy=True)

**Test Example**

This example uses a generated dataset so that it will run on automated tests
of the build system where the above datafiles do not exist.

.. testcode:: ExGenerated

    ws = CreateSampleWorkspace(binWidth = 0.1, XMin = 0, XMax = 50, XUnit = 'DeltaE')
    ws = ScaleX(ws, -25, "Add")
    LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap = True)
    ws = SofQW(ws, [0, 0.05, 8], 'Direct', 25)
    ws_DOS = ComputeIncoherentDOS(ws)

Output

.. testoutput:: ExGenerated

.. categories::

.. sourcelink::

