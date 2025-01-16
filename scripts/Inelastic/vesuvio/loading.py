# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import mtd
from mantid.api import MatrixWorkspace
from vesuvio.instrument import VESUVIO
from mantid.simpleapi import CropWorkspace, LoadVesuvio, Rebin


class VesuvioLoadHelper(object):
    """
    A helper class for loading Vesuvio data from the input of a user script.

    Attributes:
        _diff_mode       The difference mode to load runs with.
        _fit_mode        The fit mode to load runs with.
        _param_file      The instrument parameter file for loading.
        _rebin_params    The parameters to use for rebinning loaded data.
                         If none, loaded data is not rebinned.
        _load_log_files  Whether to load sample log files into loaded data.
    """

    def __init__(self, diff_mode, fit_mode, param_file, rebin_params=None, load_log_files=True):
        self.fit_mode = fit_mode
        self._diff_mode = self._parse_diff_mode(diff_mode)
        self._param_file = param_file
        self._rebin_params = rebin_params
        self._load_log_files = load_log_files
        self._instrument = VESUVIO()

    def __call__(self, runs, spectra):
        return self.load_and_crop_runs(runs, spectra)

    def load_and_crop_runs(self, runs, spectra):
        sum_spectra = self.fit_mode == "bank"
        loaded = self.load_runs(runs, spectra, sum_spectra)
        cropped = self._crop_workspace(loaded)
        return self._rebin_workspace(cropped) if self._rebin_params is not None else cropped

    def _parse_diff_mode(self, diff_mode):
        if diff_mode == "single":
            return "SingleDifference"
        elif diff_mode == "double":
            return "DoubleDifference"
        else:
            return diff_mode

    def load_runs(self, runs, spectra, sum_spectra=False):
        return LoadVesuvio(
            Filename=runs,
            Mode=self._diff_mode,
            InstrumentParFile=self._param_file,
            SpectrumList=spectra,
            SumSpectra=sum_spectra,
            LoadLogFiles=self._load_log_files,
            OutputWorkspace="loaded",
            StoreInADS=False,
        )

    def _crop_workspace(self, workspace):
        return CropWorkspace(
            InputWorkspace=workspace,
            XMin=self._instrument.tof_range[0],
            XMax=self._instrument.tof_range[1],
            OutputWorkspace="cropped",
            StoreInADS=False,
        )

    def _rebin_workspace(self, workspace):
        return Rebin(InputWorkspace=workspace, Params=self._rebin_params, OutputWorkspace="rebinned", StoreInADS=False)


class VesuvioTOFFitInput(object):
    """
    A helper class for loading and storing the specified spectra of the specified
    input sample and container runs, using the given loader.

    Attributes:
        sample_runs         The sample runs to load and store.
        container_runs      The container runs to load and store.
        sample_data         The loaded sample runs as a workspace.
        container_data      The loaded container runs as a workspace
        spectra             The spectra to load.
        _back_scattering    True if back scattering spectra are being
                            loaded, false otherwise.
    """

    def __init__(self, sample_runs, container_runs, spectra, loader):
        # Load sample and container runs
        self.spectra = _parse_spectra(loader.fit_mode, spectra, sample_runs)

        self.sample_runs, self.sample_data = self._try_load_data(sample_runs, self.spectra, loader, "Unable to load Sample Runs:")
        mtd.addOrReplace(self._tof_workspace_suffix(self.sample_runs, self.spectra), self.sample_data)

        if container_runs is not None:
            self.container_runs, self.container_data = self._try_load_data(
                container_runs, self.spectra, loader, "Unable to load Container Runs:"
            )
            mtd.addOrReplace(self._tof_workspace_suffix(self.container_runs, self.spectra), self.container_data)
        else:
            self.container_runs, self.container_data = None, None

        self._back_scattering = is_back_scattering_spectra(self.spectra)

    @property
    def spectra_number(self):
        return self.sample_data.getNumberHistograms()

    @property
    def using_back_scattering_spectra(self):
        """
        :return: True if back-scattering spectra is being used as input,
                 False otherwise.
        """
        return self._back_scattering

    def _try_load_data(self, runs, spectra, loader, error_msg):
        try:
            return self._load_data(runs, spectra, loader)
        except Exception as exc:
            raise RuntimeError(error_msg + "\n" + str(exc))

    def _load_data(self, runs, spectra, loader):
        """
        Loads the specified spectra, of the specified runs, using the
        specified loader.

        :param runs:    The runs to load - either an already loaded
                        runs workspace or an input string of runs.
        :param spectra: The spectra of the runs to load.
        :param loader:  The loader to use.
        :return:        A tuple of the name of the input workspace
                        or input string and the loaded output workspace.
        """
        if isinstance(runs, MatrixWorkspace):
            return runs.name(), runs
        else:
            return runs, loader(runs, spectra)

    def _tof_workspace_suffix(self, runs, spectra):
        """
        Creates and returns the suffix for the loaded TOF workspaces.

        :param runs:    The runs being loaded.
        :param spectra: The spectra being loaded.
        :return:        A suffix for the TOF workspaces being loaded.
        """
        return runs + "_" + spectra + "_tof"


def _parse_spectra(fit_mode, spectra, sample_runs):
    if isinstance(sample_runs, MatrixWorkspace):
        return _spectra_from_workspace(sample_runs)
    elif spectra is not None:
        return _spectra_from_string(fit_mode, spectra)
    else:
        raise RuntimeError("No spectra have been defined; unable to run fit routines.")


def _spectra_from_string(fit_mode, spectra):
    if fit_mode == "bank":
        return _parse_spectra_bank(spectra)
    else:
        if spectra == "forward":
            return "{0}-{1}".format(*VESUVIO().forward_spectra)
        elif spectra == "backward":
            return "{0}-{1}".format(*VESUVIO().backward_spectra)
        else:
            return spectra


def _spectra_from_workspace(workspace):
    first_spectrum = workspace.getSpectrum(0).getSpectrumNo()
    last_spectrum = workspace.getSpectrum(workspace.getNumberHistograms() - 1).getSpectrumNo()
    return "{0}-{1}".format(first_spectrum, last_spectrum)


def _parse_spectra_bank(spectra_bank):
    if spectra_bank == "forward":
        bank_ranges = VESUVIO().forward_banks
    elif spectra_bank == "backward":
        bank_ranges = VESUVIO()._instrument.backward_banks
    else:
        raise ValueError("Fitting by bank requires selecting either 'forward' or 'backward' for the spectra to load")
    bank_ranges = ["{0}-{1}".format(x, y) for x, y in bank_ranges]
    return ";".join(bank_ranges)


def is_back_scattering_spectra(spectra):
    """
    Checks whether the specified spectra denotes back scattering spectra.

    :param spectra: The spectra to check.
    :return:        True if the specified spectra denotes back scattering
                    spectra.
    """
    if spectra == "forward":
        return False
    elif spectra == "backward":
        return True
    else:
        try:
            first_spectrum = int(spectra.split("-")[0])
            return any([lower <= first_spectrum <= upper for lower, upper in VESUVIO().backward_banks])
        except:
            raise RuntimeError(
                "Invalid value given for spectrum range: Range must either be 'forward', 'backward' or specified with the syntax 'a-b'."
            )
