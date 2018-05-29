.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Exports the results of a :ref:`GSASIIRefineFitPeaks
<algm-GSASIIRefineFitPeaks-v1>` refinement, as well as the refinement
parameters used, to an HDF5 file indexed by bank ID. If multiple sets
of fit results are provided, then the file is divided into sub-groups
for each run number, with the **Run Number** groups indexed further by
bank ID.The results go in a sub-group of the **Bank** group called
**GSAS-II Fitting**. The subgroups of this are the following:

Refinement Parameters
#####################

Settings passed to :ref:`GSASIIRefineFitPeaks
<algm-GSASIIRefineFitPeaks-v1>` to generate this refinement.

- **RefinementMethod** - either Pawley or Rietveld refinement
- **RefineSigma** - whether sigma (Gaussian broadening term) was
  refined
- **RefineGamma** - whether gamma (Lorentzian broadening term) was
  refined
- **XMin** - the minimum TOF value used for refinement. Note this may
  not be the same as the **XMin** that you passed to
  GSASIIRefineFitPeaks, as **XMin** in GSASIIRefineFitPeaks will be
  overriden by **PawleyDMin** if the latter corresponds to a greater
  TOF value
- **XMax** - the maximum TOF value used for refinement

Additionally, if **RefinementMethod** is ``Pawley refinement``, the
two Pawley parameters will be saved:

- **PawleyDMin** - the minimum D spacing to use for refinement
- **PawleyNegativeWeight** - a weight penalty used in Pawley
  refinement

Lattice Parameters
##################

The lattice parameters of the refined structure: **alpha**, **beta**,
**gamma**, **a**, **b**, **c** and **volume**.

Profile Coefficients
####################

Only saved if either **RefineSigma** or **RefineGamma** were turned
on.

- **Gamma** - Lorentzian broadening term of the GSAS-II peak
  profile. Only saved if it was refined
- **Sigma** - Gaussian broadening term of the GSAS-II peak
  profile. Only saved if it was refined

Rwp (weighted-profile R-factor)
###############################

A measure of 'goodness of fit', as a percentage

File Structure
##############

Obtaining fit results for banks 1 and 2 of run 123456 and then saving
them with algorithm would yield the following file structure:

.. diagram:: EnggSaveGSASIIFitResultsToHDF5SingleRunHierarchy.dot

Obtaining fit results for banks 1 and 2 of run 123456 and bank 1 of
123457 and then saving them with the algorithm would yield the
following file structure:

.. diagram:: EnggSaveGSASIIFitResultsToHDF5MultiRunHierarchy.dot

Usage
-----

.. warning::

   Due to a reliance on GSAS-II, this example is not run on the build
   servers, so may not be correct. Please inform Mantid developers if
   you spot something awry

**Example - Export refinement results to a new HDF5 file:**

.. code-block:: python

   import os

   path_to_gsas = r"C:\g2conda\GSASII"

   gsas_proj_file = r"C:\mantid-data\280625.gpx"

   data_dir = r"C:\mantid-data"
   input_file = lambda file: os.path.join(data_dir, file)

   phase_file = input_file("Fe-alpha.cif")
   iparams_file = input_file("template_ENGINX_241391_236516_North_bank.prm")
   input_ws = Load(Filename=input_file("ENGINX_280625_focused_bank_1.nxs"))

   fitted_peaks, lattice_params, rwp, sigma, gamma = \
       GSASIIRefineFitPeaks(InputWorkspace=input_ws,
                            PhaseInfoFiles=phase_file,
                            InstrumentFile=iparams_file,
                            RefinementMethod="Rietveld refinement",
                            SaveGSASIIProjectFile=gsas_proj_file,
                            PathToGSASII=path_to_gsas,
                            RefineSigma=True,
                            RefineGamma=False)

   EnggSaveGSASIIFitResultsToHDF5(LatticeParams=lattice_params,
                                  Filename=r"D:\doctest.hdf5",
                                  BankID=input_ws.run()["bankid"].value,
                                  RefinementMethod="Rietveld refinement",
                                  XMin=min(fitted_peaks.readX(0)),
                                  XMax=max(fitted_peaks.readX(0)),
                                  RefineSigma=True,
                                  RefineGamma=False,
                                  Sigma=sigma,
				  Rwp=rwp)

.. categories::

.. sourcelink::
