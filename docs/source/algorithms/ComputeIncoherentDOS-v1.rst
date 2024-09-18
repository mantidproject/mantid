.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Computes the phonon density of states from an inelastic neutron
scattering measurement of a powder or polycrystalline sample,
assuming that all scattering is incoherent, using the formula
for the 1-phonon incoherent scattering function [#CHAPLOT]_:

.. math::

    S^{(1)}_{\mathrm{inc}}(Q,E) = \exp\left(-2\bar{W}(Q)\right) \frac{Q^2}{E} \left< n+\frac{1}{2}\pm\frac{1}{2} \right> \left[ \sum_k \frac{\sigma_k^{\mathrm{scatt}}}{2m_k} g_k(E) \right],

where the term in square brackets is the neutron weighted density of
states which is calculated by this algorithm, and :math:`g_k(E)` is
the partial density of states for each component (element or isotope)
:math:`k` in the material. :math:`m_k` is the relative atomic mass of the
component.

The average Debye-Waller factor :math:`\exp\left(-2\bar{W}(Q)\right)` is
calculated using an average mean-square displacement :math:`\langle u^2 \rangle`,
using :math:`W=Q^2\langle u^2\rangle/2`.

:math:`\langle u^2 \rangle` is also called the isotropic atomic displacement
parameter :math:`U_{\mathrm{iso}}` in Rietveld refinement programs
like `GSAS <https://subversion.xray.aps.anl.gov/trac/EXPGUI/wiki/GSASIntro>`_.
There is also another, related, type of atomic displacement parameter used in programs like
`FullProf <https://www.ill.eu/sites/fullprof/>`_ called :math:`B_{\mathrm{iso}}` : :math:`B_{\mathrm{iso}}=8\pi^2U_{\mathrm{iso}}` [#GSASREF]_.

The algorithm accepts both :math:`S(Q,E)` workspaces as well as
:math:`S(2\theta,E)` workspaces. In the latter case :math:`Q` values are
calculated from :math:`2\theta` and :math:`E` before applying the formula
above. Note, that ``QSumRange`` is only applicable with :math:`S(Q,E)` while
``TwoThetaSumRange`` works only for :math:`S(2\theta,E)`.

If the data has been normalised to a Vanadium standard measurement, the
output of this algorithm is the neutron weighted density of states in
milibarns/steradians per formula unit per meV (or per cm^-1). If the sample
material has been set and is found to be a pure element, then an additional
option will be enabled to calculate the DOS in states per meV (states per
cm^-1) by dividing by the scattering cross-section and multiplying by the
relative atomic mass.

Restrictions on the Input Workspace
###################################

The input workspace must have units of Momentum Transfer or Degrees and
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

**ILL  Example using S(2theta, E) as input**

.. plot::
   :include-source:

    from mantid import mtd
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt

    ws = DirectILLCollectData('ILL/IN4/087294.nxs')
    DirectILLReduction(ws, OutputWorkspace='sqw', OutputSofThetaEnergyWorkspace='stw')
    temperature = ws.run().getProperty('sample.temperature').value
    dos = ComputeIncoherentDOS('stw',  Temperature=temperature, EnergyBinning='0, Emax')

    fig, axis = plt.subplots(subplot_kw={'projection':'mantid'})
    axis.errorbar(dos)
    axis.set_title('Density of states from $S(2\\theta,W)$')
    # Uncomment the line below to show the plot.
    #fig.show()
    mtd.clear()

**Test Example**

This example uses a generated dataset so that it will run on automated tests
of the build system where the above datafiles do not exist.

.. code:: python

    ws = CreateSampleWorkspace(binWidth = 0.1, XMin = 0, XMax = 50, XUnit = 'DeltaE')
    ws = ScaleX(ws, -25, "Add")
    LoadInstrument(ws, InstrumentName='MARI', RewriteSpectraMap = True)
    ws = SofQW(ws, [0, 0.05, 8], 'Direct', 25)
    ws_DOS = ComputeIncoherentDOS(ws)

References
----------

.. [#CHAPLOT] Thermodynamic Properties of Solids, eds. Chaplot, Mittal, Choudhury, Chapter 3

.. [#GSASREF] `GSAS Manual <https://subversion.xray.aps.anl.gov/EXPGUI/gsas/all/GSAS%20Manual.pdf>`_, page 123

.. categories::

.. sourcelink::

