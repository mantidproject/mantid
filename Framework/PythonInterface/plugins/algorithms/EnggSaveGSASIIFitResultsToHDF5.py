from __future__ import (absolute_import, division, print_function)

from mantid.api import *
from mantid.kernel import *
import h5py
import numpy


class EnggSaveGSASIIFitResultsToHDF5(PythonAlgorithm):

    PROP_BANKID = "BankID"
    PROP_LATTICE_PARAMS = "LatticeParams"
    PROP_SIGMA = "Sigma"
    PROP_GAMMA = "Gamma"
    PROP_RWP = "Rwp"
    PROP_XMIN = "XMin"
    PROP_XMAX = "XMax"
    PROP_PAWLEY_DMIN = "PawleyDMin"
    PROP_PAWLEY_NEGATIVE_WEIGHT = "PawleyNegativeWeight"
    PROP_REFINE_GAMMA = "RefineGamma"
    PROP_REFINE_SIGMA = "RefineSigma"
    PROP_REFINEMENT_METHOD = "RefinementMethod"
    PROP_FILENAME = "Filename"

    BANK_GROUP_NAME = "Bank {}".format
    LATTICE_PARAMS_DATASET_NAME = "Lattice Parameters"
    REFINEMENT_PARAMS_DATASET_NAME = "Refinement Parameters"
    FIT_RESULTS_GROUP_NAME = "GSAS-II Fitting"
    LATTICE_PARAMS = ["a", "b", "c", "alpha", "beta", "gamma", "volume"]
    PAWLEY_REFINEMENT = "Pawley refinement"
    RIETVELD_REFINEMENT = "Rietveld refinement"
    RWP_DATASET_NAME = "Rwp"
    PROFILE_COEFFS_DATASET_NAME = "Profile Coefficients"

    def category(self):
        return "DataHandling"

    def name(self):
        return "EnggSaveGSASIIFitResultsToHDF5"

    def summary(self):
        return "Save input parameters and fit results from GSASIIRefineFitPeaks to an HDF5 file, indexed by bank ID"

    def PyInit(self):
        self.declareProperty(ITableWorkspaceProperty(name=self.PROP_LATTICE_PARAMS, defaultValue="",
                                                     direction=Direction.Input),
                             doc="Table workspace containing lattice parameters")

        self.declareProperty(name=self.PROP_BANKID, validator=IntMandatoryValidator(),
                             doc="ID of the bank associated with this data (1 for North, 2 for South)", defaultValue=1)

        self.declareProperty(name=self.PROP_REFINEMENT_METHOD, defaultValue=self.PAWLEY_REFINEMENT,
                             validator=StringListValidator([self.PAWLEY_REFINEMENT, self.RIETVELD_REFINEMENT]),
                             doc="Which refinement method was used")

        self.declareProperty(name=self.PROP_XMIN, defaultValue=0.0, doc="Minimum TOF value used for refinement")

        self.declareProperty(name=self.PROP_XMAX, defaultValue=0.0, doc="Maximum TOF value used for refinement")

        self.declareProperty(name=self.PROP_PAWLEY_DMIN, defaultValue=0.0,
                             doc="Minimum d spacing used for Pawley refinement")

        self.declareProperty(name=self.PROP_PAWLEY_NEGATIVE_WEIGHT, defaultValue=0.0,
                             doc="Negative weight penalty used in Pawley refinement")

        self.declareProperty(name=self.PROP_REFINE_SIGMA, defaultValue=False,
                             doc="Whether to sigma profile coefficient was refined")

        self.declareProperty(name=self.PROP_REFINE_GAMMA, defaultValue=False,
                             doc="Whether gamma profile coefficient was refined")

        self.declareProperty(name=self.PROP_SIGMA, defaultValue=0.0, doc="GSAS-II profile coefficient sigma")

        self.declareProperty(name=self.PROP_GAMMA, defaultValue=0.0, doc="GSAS-II profile coefficient gamma")

        self.declareProperty(name=self.PROP_RWP, defaultValue=0.0,
                             doc="Weighted profile R-factor, 'goodness-of-fit' measure")

        self.declareProperty(FileProperty(name=self.PROP_FILENAME, defaultValue="", action=FileAction.Save,
                                          extensions=[".hdf5", ".h5", ".hdf"]), doc="HDF5 file to save to")

    def PyExec(self):
        output_file_name = self.getProperty(self.PROP_FILENAME).value

        with h5py.File(output_file_name, "a") as output_file:
            bankID = self.getProperty(self.PROP_BANKID).value

            bank_group = output_file.require_group(self.BANK_GROUP_NAME(bankID))
            if self.FIT_RESULTS_GROUP_NAME in bank_group:
                del bank_group[self.FIT_RESULTS_GROUP_NAME]

            fit_results_group = bank_group.create_group(self.FIT_RESULTS_GROUP_NAME)

            self._save_refinement_params(fit_results_group)
            self._save_lattice_params(fit_results_group)
            self._save_rwp(fit_results_group)
            self._save_profile_coefficients(fit_results_group)

    def _save_lattice_params(self, results_group):
        lattice_params_dataset = results_group.create_dataset(name=self.LATTICE_PARAMS_DATASET_NAME, shape=(1,),
                                                              dtype=[(param, "f") for param in self.LATTICE_PARAMS])
        lattice_params = self.getProperty(self.PROP_LATTICE_PARAMS).value.row(0)

        lattice_params_dataset[0] = tuple(lattice_params[param] for param in self.LATTICE_PARAMS)

    def _save_profile_coefficients(self, results_group):
        refine_sigma = self.getProperty(self.PROP_REFINE_SIGMA).value
        refine_gamma = self.getProperty(self.PROP_REFINE_GAMMA).value

        if refine_sigma or refine_gamma:
            if refine_sigma and refine_gamma:
                dtype = [(self.PROP_SIGMA, "f"), (self.PROP_GAMMA, "f")]
                values = self.getProperty(self.PROP_SIGMA).value, self.getProperty(self.PROP_GAMMA).value
            elif refine_sigma:
                dtype = [(self.PROP_SIGMA, "f")]
                values = self.getProperty(self.PROP_SIGMA).value,
            elif refine_gamma:
                dtype = [(self.PROP_GAMMA, "f")]
                values = self.getProperty(self.PROP_GAMMA).value,

            coefficients_dataset = results_group.create_dataset(name=self.PROFILE_COEFFS_DATASET_NAME, shape=(1,),
                                                                dtype=dtype)
            coefficients_dataset[0] = values

    def _save_refinement_params(self, results_group):
        refinement_params_dtype = [(self.PROP_REFINEMENT_METHOD, "S19"),
                                   (self.PROP_REFINE_SIGMA, "b"),
                                   (self.PROP_REFINE_GAMMA, "b"),
                                   (self.PROP_XMIN, "f"),
                                   (self.PROP_XMAX, "f")]

        refinement_method = self.getProperty(self.PROP_REFINEMENT_METHOD).value
        refinement_params_list = [refinement_method,
                                  self.getProperty(self.PROP_REFINE_SIGMA).value,
                                  self.getProperty(self.PROP_REFINE_GAMMA).value,
                                  self.getProperty(self.PROP_XMIN).value,
                                  self.getProperty(self.PROP_XMAX).value]

        if refinement_method == self.PAWLEY_REFINEMENT:
            refinement_params_dtype += [(self.PROP_PAWLEY_DMIN, "f"),
                                        (self.PROP_PAWLEY_NEGATIVE_WEIGHT, "f")]
            refinement_params_list += [self.getProperty(self.PROP_PAWLEY_DMIN).value,
                                       self.getProperty(self.PROP_PAWLEY_NEGATIVE_WEIGHT).value]

        refinement_params_dataset = results_group.create_dataset(name=self.REFINEMENT_PARAMS_DATASET_NAME, shape=(1,),
                                                                 dtype=refinement_params_dtype)

        refinement_params_dataset[0] = tuple(refinement_params_list)

    def _save_rwp(self, results_group):
        results_group.create_dataset(self.RWP_DATASET_NAME, data=numpy.array(self.getProperty(self.PROP_RWP).value))


AlgorithmFactory.subscribe(EnggSaveGSASIIFitResultsToHDF5)
