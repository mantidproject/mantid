# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import mtd, AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm
from mantid.kernel import FloatArrayProperty, IntArrayProperty, StringArrayProperty, StringListValidator
import h5py
import numpy


class EnggSaveGSASIIFitResultsToHDF5(PythonAlgorithm):
    PROP_BANKIDS = "BankIDs"
    PROP_LATTICE_PARAMS = "LatticeParamWorkspaces"
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
    PROP_RUN_NUMBERS = "RunNumbers"
    PROP_FILENAME = "Filename"

    BANK_GROUP_NAME = "Bank {}".format
    RUN_GROUP_NAME = "Run {}".format
    LATTICE_PARAMS_GROUP_NAME = "Lattice Parameters"
    REFINEMENT_PARAMS_GROUP_NAME = "Refinement Parameters"
    FIT_RESULTS_GROUP_NAME = "GSAS-II Fitting"
    LATTICE_PARAMS = ["a", "b", "c", "alpha", "beta", "gamma", "volume"]
    PAWLEY_REFINEMENT = "Pawley refinement"
    RIETVELD_REFINEMENT = "Rietveld refinement"
    RWP_DATASET_NAME = "Rwp"
    PROFILE_COEFFS_GROUP_NAME = "Profile Coefficients"
    WRONG_NUMBER_PARAM_MSG = "One {} value must be supplied for every run {}"

    def category(self):
        return "DataHandling"

    def name(self):
        return "EnggSaveGSASIIFitResultsToHDF5"

    def summary(self):
        return "Save input parameters and fit results from GSASIIRefineFitPeaks to an HDF5 file, indexed by bank ID"

    def validateInputs(self):
        issues = {}

        lattice_param_wss = self.getProperty(self.PROP_LATTICE_PARAMS).value
        for ws_name in lattice_param_wss:
            if not mtd.doesExist(ws_name):
                issues[self.PROP_LATTICE_PARAMS] = "The workspace {} does not exist".format(ws_name)

        num_runs = len(lattice_param_wss)

        issues.update(self._wrong_number_warning_dict(self.PROP_BANKIDS, num_runs))
        issues.update(self._wrong_number_warning_dict(self.PROP_RWP, num_runs))

        refine_sigma = self.getProperty(self.PROP_REFINE_SIGMA).value
        if refine_sigma:
            issues.update(self._wrong_number_warning_dict(self.PROP_SIGMA, num_runs, " when {} is True".format(self.PROP_REFINE_SIGMA)))
        refine_gamma = self.getProperty(self.PROP_REFINE_GAMMA).value
        if refine_gamma:
            issues.update(self._wrong_number_warning_dict(self.PROP_GAMMA, num_runs, " when {} is True".format(self.PROP_REFINE_GAMMA)))
        if num_runs > 1:
            issues.update(self._wrong_number_warning_dict(self.PROP_RUN_NUMBERS, num_runs, " when saving multiple refinement results"))

        return issues

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name=self.PROP_LATTICE_PARAMS), doc="Table workspace containing lattice parameters")

        self.declareProperty(IntArrayProperty(name=self.PROP_RUN_NUMBERS), doc="The run number of each fitted run")

        self.declareProperty(
            IntArrayProperty(name=self.PROP_BANKIDS),
            doc="Bank IDs of the workspaces a fit was run on " "(1 for each workspace, in order. 1 for North, 2 for South)",
        )

        self.declareProperty(
            name=self.PROP_REFINEMENT_METHOD,
            defaultValue=self.PAWLEY_REFINEMENT,
            validator=StringListValidator([self.PAWLEY_REFINEMENT, self.RIETVELD_REFINEMENT]),
            doc="Which refinement method was used",
        )

        self.declareProperty(name=self.PROP_XMIN, defaultValue=0.0, doc="Minimum TOF value used for refinement")

        self.declareProperty(name=self.PROP_XMAX, defaultValue=0.0, doc="Maximum TOF value used for refinement")

        self.declareProperty(name=self.PROP_PAWLEY_DMIN, defaultValue=0.0, doc="Minimum d spacing used for Pawley refinement")

        self.declareProperty(
            name=self.PROP_PAWLEY_NEGATIVE_WEIGHT, defaultValue=0.0, doc="Negative weight penalty used in Pawley refinement"
        )

        self.declareProperty(name=self.PROP_REFINE_SIGMA, defaultValue=False, doc="Whether to sigma profile coefficient was refined")

        self.declareProperty(name=self.PROP_REFINE_GAMMA, defaultValue=False, doc="Whether gamma profile coefficient was refined")

        self.declareProperty(
            FloatArrayProperty(name=self.PROP_SIGMA),
            doc="GSAS-II profile coefficient sigma, " "one for each run (or none if sigma wasn't refined)",
        )

        self.declareProperty(
            FloatArrayProperty(name=self.PROP_GAMMA),
            doc="GSAS-II profile coefficient gamma, " "one for each run (or none if gamma wasn't refined)",
        )

        self.declareProperty(
            FloatArrayProperty(name=self.PROP_RWP), doc="Weighted profile R-factor, 'goodness-of-fit' measure. " "One for each run"
        )

        self.declareProperty(
            FileProperty(name=self.PROP_FILENAME, defaultValue="", action=FileAction.Save, extensions=[".hdf5", ".h5", ".hdf"]),
            doc="HDF5 file to save to",
        )

    def PyExec(self):
        output_file_name = self.getProperty(self.PROP_FILENAME).value
        lattice_param_ws_names = self.getProperty(self.PROP_LATTICE_PARAMS).value
        run_numbers = self.getProperty(self.PROP_RUN_NUMBERS).value
        bankIDs = self.getProperty(self.PROP_BANKIDS).value
        rwps = self.getProperty(self.PROP_RWP).value
        refine_sigma = self.getProperty(self.PROP_REFINE_SIGMA).value
        refine_gamma = self.getProperty(self.PROP_REFINE_GAMMA).value
        sigmas = self.getProperty(self.PROP_SIGMA).value
        gammas = self.getProperty(self.PROP_GAMMA).value

        with h5py.File(output_file_name, "a") as output_file:
            for i, ws_name in enumerate(lattice_param_ws_names):
                if len(lattice_param_ws_names) > 1:
                    top_level_group = output_file.require_group(self.RUN_GROUP_NAME(run_numbers[i]))
                else:
                    top_level_group = output_file

                bank_group = top_level_group.require_group(self.BANK_GROUP_NAME(bankIDs[i]))

                if self.FIT_RESULTS_GROUP_NAME in bank_group:
                    del bank_group[self.FIT_RESULTS_GROUP_NAME]

                fit_results_group = bank_group.create_group(self.FIT_RESULTS_GROUP_NAME)

                self._save_refinement_params(fit_results_group)
                self._save_lattice_params(fit_results_group, lattice_params_ws=mtd[ws_name])
                self._save_rwp(fit_results_group, rwp=rwps[i])
                self._save_profile_coefficients(
                    fit_results_group, refine_sigma=refine_sigma, refine_gamma=refine_gamma, sigmas=sigmas, gammas=gammas, index=i
                )

    def _wrong_number_warning_dict(self, prop_name, num_runs, condition=""):
        prop = self.getProperty(prop_name).value
        if len(prop) != num_runs:
            return {prop_name: self.WRONG_NUMBER_PARAM_MSG.format(prop_name, condition)}

        return {}

    def _save_lattice_params(self, results_group, lattice_params_ws):
        lattice_params_group = results_group.create_group(name=self.LATTICE_PARAMS_GROUP_NAME)
        lattice_params = lattice_params_ws.row(0)

        for param_name in self.LATTICE_PARAMS:
            lattice_params_group.create_dataset(name=param_name, data=lattice_params[param_name])

    def _save_profile_coefficients(self, results_group, refine_sigma, refine_gamma, sigmas, gammas, index):
        if refine_sigma or refine_gamma:
            coeffs_group = results_group.create_group(name=self.PROFILE_COEFFS_GROUP_NAME)

            if refine_sigma:
                coeffs_group.create_dataset(name="Sigma", data=sigmas[index])
            if refine_gamma:
                coeffs_group.create_dataset(name="Gamma", data=gammas[index])

    def _copy_property_to_dataset(self, group, property_name):
        group.create_dataset(name=property_name, data=self.getProperty(property_name).value)

    def _save_refinement_params(self, results_group):
        refinement_params_group = results_group.create_group(name=self.REFINEMENT_PARAMS_GROUP_NAME)

        mandatory_properties = {self.PROP_REFINEMENT_METHOD, self.PROP_REFINE_SIGMA, self.PROP_REFINE_GAMMA, self.PROP_XMIN, self.PROP_XMAX}

        for prop_name in mandatory_properties:
            self._copy_property_to_dataset(refinement_params_group, prop_name)

        if self.getProperty(self.PROP_REFINEMENT_METHOD).value == self.PAWLEY_REFINEMENT:
            self._copy_property_to_dataset(refinement_params_group, self.PROP_PAWLEY_DMIN)
            self._copy_property_to_dataset(refinement_params_group, self.PROP_PAWLEY_NEGATIVE_WEIGHT)

    def _save_rwp(self, results_group, rwp):
        results_group.create_dataset(self.RWP_DATASET_NAME, data=numpy.array(rwp))


AlgorithmFactory.subscribe(EnggSaveGSASIIFitResultsToHDF5)
