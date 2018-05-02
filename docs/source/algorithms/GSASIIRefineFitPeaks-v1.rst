.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. warning::

   This algorithm requires GSAS-II to be installed on your computer. A
   version of GSAS-II containing the module GSASIIscriptable (added in
   April 2017) is required. See Installing_GSASII_
   for how to get the correct version of GSAS-II.

Uses `GSAS-II <https://subversion.xray.aps.anl.gov/trac/pyGSAS>`_
[TobyVonDreele2013]_ as external software to fit peaks to a powder /
engineering diffraction pattern. Here the process of peak fitting is
in the context of Rietveld / Pawley / Le Bail analysis [LeBail2005]_

The algorithm supports two refinement methods: Pawley refinement and
Rietveld refinement. The use of this algorithm is very close to the
examples described in these two GSAS-II tutorials: `Rietveld fitting /
CW Neutron Powder fit for Yttrium-Iron Garnet
<https://subversion.xray.aps.anl.gov/pyGSAS/Tutorials/CWNeutron/Neutron%20CW%20Powder%20Data.htm>`_,
and `Getting started / Fitting individual peaks & autoindexing
<https://subversion.xray.aps.anl.gov/pyGSAS/Tutorials/FitPeaks/Fit%20Peaks.htm>`_,
The functionality of this algorithm is based on the `powder
calculation module
<https://subversion.xray.aps.anl.gov/pyGSAS/sphinxdocs/build/html/GSASIIpwd.html>`_
and the `structure routines
<https://subversion.xray.aps.anl.gov/pyGSAS/sphinxdocs/build/html/GSASIIstruc.html>`_
of GSAS-II.

The refinement methods of this algorithm are equivalent to the
function "Calculate / Refine" from the main menu of the GSAS-II GUI.

The main inputs required are histogram data, an instrument definition
parameter (in GSAS format, readable by GSAS-II), phase information and
various parameters for the fitting/refinement process.

The phase information must be provided in `CIF format
(Crystallographic Information File)
<https://en.wikipedia.org/wiki/Crystallographic_Information_File>`_.
When phase information is available the algorithm will output the
lattice parameters in a table workspace. The values are given for the
the full set of lattice parameters (three lattice constants, three
angles, and volume in this sequence: a, b, c, alpha, beta, gamma,
volume). The a,b, and c values are given in Angstroms
(:math:`\mathrm{\AA{}}`). The angles are given in degrees, and the
volume in :math:`\mathrm{\AA{}}^3`.

The algorithm provides goodness-of-fit estimates in the outputs *GoF*
and *Rwp* or weighted profile R-factor [Toby2008]_. The *Rwp* is given
as a percentage value.

When Pawley refinement is selected as refinement method the flag for
histogram scale factor refinement is disabled, as recommended in the
`GSAS-II documentation
<https://subversion.xray.aps.anl.gov/pyGSAS/trunk/help/gsasII.html>`_,
as this cannot be refined simultaneously with the Pawley reflection
intensities.

The GSAS-II Rietveld/Pawley refinement process writes lattice
parameters and extensive additional information in an output file with
the same name as the output GSAS-II project file but with extension
".lst". This is noted in a log message that specifies where the file
has been written (next to the output project file).

.. _Installing_GSASII:

Installing GSAS-II
------------------

On Windows, run either :code:`scripts\GSAS-II\install_gsas_latest.bat`
or :code:`scripts\GSAS-II\install_gsas_vetted.bat`. Use the former for
the latest version of GSAS-II and the latter for the most recent
version to have been manually verified to work with
GSASIIRefineFitPeaks.  This will install GSAS-II to your current
working drive (usually C) in a directory called :code:`g2conda`.

On Linux, from :code:`scripts\GSAS-II` run :code:`python install_gsas_proxy.py`.
Optionally, you can supply a desired revision number (defaults to the
latest) with the :code:`-v` flag and a directory (defaults to
:code:`/`) to install GSAS-II to with the :code:`-d` flag.


*References*:

.. [LeBail2005] Le Bail, A (2005). "Whole Powder Pattern Decomposition Methods and
                Applications: A Retrospection". Powder Diffraction 20(4): 316-326.

.. [TobyVonDreele2013] Toby, B. H., & Von Dreele, R. B. (2013). "GSAS-II: the
                       genesis of a modern open-source all purpose crystallography
                       software package". Journal of Applied Crystallography, 46(2),
                       544-549.

.. [Toby2008] Toby, B. H. (2008). "R factors in Rietveld analysis: How good is good
              enough?". Powder Diffraction, 21(1), 67-70.

Usage
-----

.. warning::

   Take these usage examples with a pinch of salt, as they are not
   tested for correctness on our servers, due to the requirement to
   have GSAS-II installed. Please contact the Mantid developers if
   something is awry.
   
**Example - Pawley refinement of lattice parameters from a diffraction spectrum**

.. code-block:: python

   # You would normally generate the focused file using the Engg GUI or,
   # alternatively, with commands like these:
   #
   # ws_to_focus = Load('ENGINX00256663-256675')
   # wks_ceria = Load('ENGINX00255924')
   # wks_vana = Load('ENGINX00254854')
   # # Using default expected peaks for Ceria
   # difa, difc, tzero, peaks_tbl = EnggCalibrate(InputWorkspace=wks_ceria, VanadiumWorkspace=wks_vana, Bank='North')
   # wks_focused = EnggFocus(InputWorkspace=wks, VanadiumWorkspace=wks_vana, Bank='North')
   # SaveNexus(InputWorkspace=wks_focused, Filename='focused_bank1_ENGINX00256663.nxs')
   #
   wks = Load('focused_bank1_ENGINX00256663.nxs')
   peaks, lattice_params, rwp, sigma, gamma = \
       GSASIIRefineFitPeaks(InputWorkspace=wks,
                            RefinementMethod="Pawley refinement",
                            InstrumentFile='template_ENGINX_241391_236516_North_bank.prm',
                            PhaseInfoFiles='Fe-alpha.cif,Fe-gamma.cif',
                            PathToGSASII='/home/user/g2conda/GSASII',
                            RefineSigma=True,
                            RefineGamma=True,
                            SaveGSASIIProjectFile='example_gsas2_project.gpx',
                            OutputWorkspace="FittedPeaks")
   print("Weighted profile R-factor (Rwp): {0:.5f}".format(rwp))
   print("Lattice parameters, a: {a}, b: {b}, c: {c}, alpha: {alpha}, beta: {beta}, gamma: {gamma}, "
         "Volume: {volume:.3f}".format(**lattice_params.row(0)))
   print("Sigma={}, Gamma={}".format(sigma, gamma))

Output:

.. code-block:: none

    Weighted profile R-factor (Rwp): 77.75515
    Lattice parameters, a: 2.8665, b: 2.8665, c: 2.8665, alpha: 90.0, beta: 90.0, gamma: 90.0, Volume: 23.554
    Sigma=81.0939, Gamma=0.1855

**Example - Rietveld refinement of lattice parameters from a diffraction spectrum**

.. code-block:: python

   wks=Load('focused_bank1_ENGINX00256663.nxs')
   peaks, lattice_params, rwp, sigma, gamma = \
       GSASIIRefineFitPeaks(InputWorkspace=wks,
                            RefinementMethod='Rietveld refinement',
                            InstrumentFile='template_ENGINX_241391_236516_North_bank.prm',
                            PhaseInfoFiles='Fe-alpha.cif,Fe-gamma.cif',
                            PathToGSASII='/home/user/g2conda/GSASII',
                            SaveGSASIIProjectFile='example_gsas2_project.gpx',
   print("Weighted profile R-factor (Rwp): {0:.5f}".format(rwp))
   print("Lattice parameters, a: {a}, b: {b}, c: {c}, alpha: {alpha}, beta: {beta}, gamma: {gamma}, "
         "Volume: {volume:.3f}".format(**lattice_params.row(0)))
   print("Sigma={}, Gamma={}".format(sigma, gamma))

Output:

.. code-block:: none

    Weighted profile R-factor (Rwp): 77.75499
    Lattice parameters, a: 2.8665, b: 2.8665, c: 2.8665, alpha: 90.0, beta: 90.0, gamma: 90.0, Volume: 23.554
    Sigma=81.0939, Gamma=0.1855

.. categories::

.. sourcelink::
