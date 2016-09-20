.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is experimental and at the moment is being developed
   for a specific technique. It might be changed, renamed or even
   removed without a notification, should instrument scientists decide
   to do so.

.. warning::

   This algorithm requires GSAS-II, with custom modifications to
   enable it to be used from Mantid. Please contact the Mantid
   developers for details. The GSAS-II installation instructions are
   available from the `GSAS-II website
   <https://subversion.xray.aps.anl.gov/trac/pyGSAS>`_.

Uses `GSAS-II <https://subversion.xray.aps.anl.gov/trac/pyGSAS>`_
[TobyVonDreele2013]_ as external software to fit peaks to a powder /
engineering diffraction pattern. Here the process of peak fitting is
in the context of Rietveld / Pawley / Le Bail analysis [LeBail2005]_

The algorithm supports three refinement or fitting methods: Pawley
refinement, Rietveld refinement, and single peak fitting (or "Peaks
List" of GSAS-II). The first two methods of this algorithm implement
whole diffraction pattern fitting whereas the third method fits peaks
individually.  The use of this algorithm is very close to the examples
described in these two GSAS-II tutorials: `Rietveld fitting / CW
Neutron Powder fit for Yttrium-Iron Garnet
<https://subversion.xray.aps.anl.gov/pyGSAS/Tutorials/CWNeutron/Neutron%20CW%20Powder%20Data.htm>`_,
and `Getting started / Fitting individual peaks & autoindexing
<https://subversion.xray.aps.anl.gov/pyGSAS/Tutorials/FitPeaks/Fit%20Peaks.htm>`_,
The functionality of this algorithm is based on the `powder
calculation module
<https://subversion.xray.aps.anl.gov/pyGSAS/sphinxdocs/build/html/GSASIIpwd.html>`_
and the `structure routines
<https://subversion.xray.aps.anl.gov/pyGSAS/sphinxdocs/build/html/GSASIIstruc.html>`_
of GSAS-II.

To run this algorithm GSAS-II must be installed and it must be
available for importing from the Mantid Python interpreter. This
algorithm requires a modified version of GSAS-II. Please contact the
developers for details.

The methods "Pawley refinement" and "Rietveld refinement" of this
algorithm are equivalent to the function "Calculate / Refine" from the
main menu of the GSAS-II GUI.  The method "Peak fitting" is equivalent
to the "Peak Fitting / Peak fit" action of the "Peaks List" window
menu of the GSAS-II GUI.

The main inputs required are histogram data, an instrument definition
parameter (in GSAS format, readable by GSAS-II), and various
parameters for the fitting/refinement process. Phase information is
also required to use the Pawley and Rietveld refinement.

The phase information must be provided in `CIF format
(Crystallographic Information File)
<https://en.wikipedia.org/wiki/Crystallographic_Information_File>`_.
When phase information is available and the Rietveld/Pawley method is
used the algorithm will output the lattice parameters in a table
workspace. The values are given for the the full set of lattice
parameters (three lattice constants, three angles, and volume in this
sequence: a, b, c, alpha, beta, gamma, volume). The a,b, and c values
are given in Angstroms (:math:`\mathrm{\AA{}}`). The angles are given
in degrees, and the volume in :math:`\mathrm{\AA{}}^3`.

The algorithm provides goodness-of-fit estimates in the outputs *GoF*
and *Rwp* or weighted profile R-factor [Toby2008]_. The *Rwp* is given
as a percentage value.

Note that the option to save the GSAS-II project file
(*SaveGSASIIProjectFile*) is mandatory. This is a requirement of
GSAS-II. These project files can be opened in the GSAS-II GUI for
further processing and analysis of the data.

When Pawley refinement is selected as refinement method the flag for
histogram scale factor refinement is disabled, as recommended in the
`GSAS-II documentation
<https://subversion.xray.aps.anl.gov/pyGSAS/trunk/help/gsasII.html>`_,
as this cannot be refined simultaenously with the Pawley reflection
intensities.

The GSAS-II Rietveld/Pawley refinement process writes lattice
parameters and extensive additional information in an output file with
the same name as the output GSAS-II project file but with extension
".lst". This is noted in a log message that specifies where the file
has been written (next to the output project file).

When fitting individual peaks using the peak fitting method (not using
Rietveld/Pawley refinement), the algorithm only supports peaks with
shape of type back-to-back exponential convoluted with pseudo-voigt
(BackToBackExponentialPV). It is possible to enable the refinement of
the different function parameters via several properties (RefineAlpha,
RefineSigma, etc.). The fitted peak parameters are given in an output
table with as many rows as peaks have been found. The columns of the
table give the parameters fitted, similarly to the information found
in the "Peaks List" window of the GSAS-II GUI. These results are
printed in the log messages as well.

For fitting single peaks, one at a time, see also :ref:`EnggFitPeaks
<algm-EnggFitPeaks>`. For other algorithms that implement different
variants of whole diffraction pattern refinement and fitting see also
:ref:`PawleyFit <algm-PawleyFit>` and :ref:`LeBailFit
<algm-LeBailFit>`.


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

**Example - Pawley refinement of lattice parameters from a diffraction spectrum**

.. code-block:: python

   # You would normally generate the focused file using the Engg GUI or,
   # alternatively, with commands like these:
   #
   # wks = Load('ENGINX00256663-256675')
   # wks_ceria = Load('ENGINX00255924')
   # wks_vana = Load('ENGINX00254854')
   # # Using default expected peaks for Ceria
   # difa, difc, tzero, peaks_tbl = EnggCalibrate(InputWorkspace=wks_ceria, VanadiumWorkspace=wks_vana, Bank='North')
   # wks_focused = EnggFocus(InputWorkspace=wks, VanadiumWorkspace=wks_vana, Bank='North')
   # SaveNexus(InputWorkspace=wks_focused, Filename='focused_bank1_ENGINX00256663.nxs')
   #
   wks=Load('focused_bank1_ENGINX00256663.nxs')
   GoF, Rwp, lattice_tbl = GSASIIRefineFitPeaks(InputWorkspace=wks,
                                                InstrumentFile='ENGINX_255924_254854_North_bank.par',
                                                PhaseInfoFile='FE_ALPHA.cif',
                                                PathToGSASII='/home/user/gsas',
                                                SaveGSASIIProjectFile='example_gsas2_project',
                                                LatticeParameters='lattice_tbl')
   print "Goodness of fit coefficient: {0:.5f}".format(GoF)
   print "Weighted profile R-factor (Rwp): {0:.5f}".format(Rwp)
   print ("Lattice parameters, a: {a}, b: {b}, c: {c}, alpha: {alpha}, beta: {beta}, gamma: {gamma}, "
          "Volume: {volume:.3f}".format(**lattice_tbl.row(0)))

Output:

.. code-block:: none

    Goodness of fit coefficient: 3.63591
    Weighted profile R-factor (Rwp): 77.27831
    Lattice parameters, a: 2.8665, b: 2.8665, c: 2.8665, alpha: 90.0, beta: 90.0, gamma: 90.0, Volume: 23.554

**Example - Rietveld refinement of lattice parameters from a diffraction spectrum**

.. code-block:: python

   wks=Load('focused_bank1_ENGINX00256663.nxs')
   GoF, Rwp, lattice_tbl = GSASIIRefineFitPeaks(InputWorkspace=wks,
                                                Method='Rietveld refinement',
                                                InstrumentFile='ENGINX_255924_254854_North_bank.par',
                                                PhaseInfoFile='FE_ALPHA.cif',
                                                PathToGSASII='/home/user/gsas',
                                                SaveGSASIIProjectFile='example_gsas2_project',
                                                LatticeParameters='lattice_tbl')
   print "Goodness of fit coefficient: {0:.5f}".format(GoF)
   print "Weighted profile R-factor (Rwp): {0:.5f}".format(Rwp)
   print ("Lattice parameters, a: {a}, b: {b}, c: {c}, alpha: {alpha}, beta: {beta}, gamma: {gamma}, "
          "Volume: {volume:.3f}".format(**lattice_tbl.row(0)))

Output:

.. code-block:: none

    Goodness of fit coefficient: 3.62483
    Weighted profile R-factor (Rwp): 77.03530
    Lattice parameters, a: 2.8665, b: 2.8665, c: 2.8665, alpha: 90.0, beta: 90.0, gamma: 90.0, Volume: 23.554

**Example - Fit several peaks from a diffraction spectrum**

.. code-block:: python

   wks=Load('focused_bank1_ENGINX00256663.nxs')
   params_tbl_name = 'tbl_fitted_params'
   GoF, Rwp, lattice_tbl = GSASIIRefineFitPeaks(InputWorkspace=wks,
                                                Method='Peak fitting',
                                                FittedPeakParameters=params_tbl_name,
                                                InstrumentFile='ENGINX_255924_254854_North_bank.par',
                                                PhaseInfoFile='FE_ALPHA.cif',
                                                PathToGSASII='/home/user/mantid-repos/gsas',
                                                SaveGSASIIProjectFile='test_gsas2_project',
                                                FittedPeakParameters=params_tbl_name)
   tbl_fitted_params = mtd[params_tbl_name]
   print "Fitted {0} peaks.".format(tbl_fitted_params.rowCount())
   print ("Parameters of the first peak. Center: {Center:.6g}, intensity: {Intensity:.5f}, "
          "alpha: {Alpha:.5f}, beta: {Beta:.5f}, sigma: {Sigma:.5f}, gamma: {Gamma:.5f}".
          format(**tbl_fitted_params.row(0)))

Output:

.. code-block:: none

    Fitted 18 peaks.
    Parameters of the first peak. Center: 38563.8, intensity: 26.22137, alpha: 0.13125, beta: 0.01990, sigma: 125475.11036, gamma: -6681.38965

.. categories::

.. sourcelink::
