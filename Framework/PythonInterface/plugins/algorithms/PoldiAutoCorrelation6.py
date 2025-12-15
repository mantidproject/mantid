# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm, Progress
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator, UnitParams, StringListValidator
import numpy as np
from plugins.algorithms.poldi_utils import (
    get_instrument_settings_from_log,
    get_max_tof_from_chopper,
    get_dspac_limits,
    get_final_dspac_array,
)
from joblib import Parallel, delayed
from functools import reduce
from operator import iadd
from itertools import islice
from multiprocessing import cpu_count


class PoldiAutoCorrelation(PythonAlgorithm):
    def category(self):
        return "SINQ\\Poldi"

    def name(self):
        return "PoldiAutoCorrelation"

    def summary(self):
        return "Performs correlation analysis of POLDI 2D-data."

    def version(self):
        return 6

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", defaultValue="", direction=Direction.Input),
            doc="The workspace containing the input data",
        )
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Output workspace containing the correlation spectrum."
        )
        positive_float_validator = FloatBoundedValidator(lower=0.0)
        self.declareProperty(
            "WavelengthMin",
            defaultValue=1.1,
            direction=Direction.Input,
            validator=positive_float_validator,
            doc="Minium wavelength to consider.",
        )
        self.declareProperty(
            "WavelengthMax",
            defaultValue=5.0,
            direction=Direction.Input,
            validator=positive_float_validator,
            doc="Maximum wavelength to consider.",
        )
        self.declareProperty(
            "InterpolationMethod",
            defaultValue="Linear",
            direction=Direction.Input,
            validator=StringListValidator(["Linear", "Nearest"]),
            doc="Interpolation used when adding intensity to a given bin in correlation function - "
            "'Nearest' is quicker but potentially less accurate.",
        )
        self.declareProperty(
            "NGroups",
            defaultValue=1,
            direction=Direction.Input,
            validator=IntBoundedValidator(lower=1),
            doc="Number of groups to split poldi detectors into (returns a spectrum per group)",
        )

    def validateInputs(self):
        issues = dict()
        if self.getProperty("WavelengthMax").value <= self.getProperty("WavelengthMin").value:
            issues["WavelengthMax"] = "WavelengthMax must be greater than WavelengthMin."
        ws = self.getProperty("InputWorkspace").value
        if ws is None:
            # group workspace
            return issues
        if ws.getNumberHistograms() == 400:
            issues["InputWorkspace"] = (
                "InputWorkspace corresponds to the old 1D wire detector geometry on POLDI, "
                "please use PoldiAutoCorrelation with keyword argument Version=5."
            )
        has_log = ws.run().hasProperty("chopperspeed")
        has_chopper = ws.getInstrument().getComponentByName("chopper") is not None
        if not has_log or not has_chopper:
            issues["InputWorkspace"] = "InputWorkspace must have chopper component and chopperspeed log."
        return issues

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        lambda_min = self.getProperty("WavelengthMin").value
        lambda_max = self.getProperty("WavelengthMax").value
        ngroups = self.getProperty("NGroups").value

        cycle_time, slit_offsets, t0_const, l1_chop = get_instrument_settings_from_log(ws)
        # get detector positions from IDF
        si = ws.spectrumInfo()
        nspec = ws.getNumberHistograms()
        tths = np.array([si.twoTheta(ispec) for ispec in range(nspec)])
        l2s = np.asarray([si.l2(ispec) for ispec in range(nspec)])
        l1 = si.l1()
        # determine npulses to include in calc
        time_max = get_max_tof_from_chopper(l1, l1_chop, l2s, tths, lambda_max) + slit_offsets[-1]
        npulses = int(time_max // cycle_time)
        # get final d-spacing array based on detector limits and wavelength range
        # in actuality not all d-spacings will be measured within the wavelength range in every pixel
        # but this is a small effect as detector doesn't cover much two-theta range
        dspac_min, dspac_max = get_dspac_limits(tths.min(), tths.max(), lambda_min, lambda_max)
        bin_width = ws.readX(0)[1] - ws.readX(0)[0]
        dspacs = get_final_dspac_array(bin_width, dspac_min, dspac_max, time_max)[:, None]
        # perform auto-correlation (Eq. 7 in POLDI concept paper)
        ipulses = np.arange(npulses)[:, None]
        offsets = (ipulses * cycle_time + slit_offsets + t0_const).flatten()
        # get time-of-flight from chopper to detector for neutron corresponding to d=1Ang
        path_length_ratio = (l2s + l1 - l1_chop) / (l2s + l1)
        tof_d1Ang = np.asarray([si.diffractometerConstants(ispec)[UnitParams.difc] * path_length_ratio[ispec] for ispec in range(nspec)])
        # loop over spectra and add to intermediate correlation
        progress = Progress(self, start=0.0, end=1.0, nreports=nspec)
        if self.getProperty("InterpolationMethod").value == "Linear":
            do_autocorr = _autocorr_spec_linear
        else:
            do_autocorr = _autocorr_spec_nearest
        generator = Parallel(n_jobs=min(4, cpu_count()), prefer="threads", return_as="generator")(
            delayed(do_autocorr)(ws.readY(ispec), tof_d1Ang[ispec], dspacs, offsets, bin_width, progress) for ispec in range(nspec)
        )
        # sum over spectra in each group
        corr = np.zeros((ngroups, len(dspacs)))
        nspectra_per_group = nspec // ngroups
        for igroup in range(ngroups - 1):
            corr[igroup, :] = _sum_over_intermediate_correlation_func(islice(generator, nspectra_per_group))
        # eval last grouping with remainder of generator
        corr[ngroups - 1, :] = _sum_over_intermediate_correlation_func(generator)
        ws_corr = self.exec_child_alg(
            "CreateWorkspace", DataX=dspacs, DataY=corr, UnitX="dSpacing", YUnitLabel="Intensity (a.u.)", NSpec=ngroups, ParentWorkspace=ws
        )
        ws_corr = self.exec_child_alg("ConvertUnits", InputWorkspace=ws_corr, Target="MomentumTransfer")
        self.setProperty("OutputWorkspace", ws_corr)

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props


def _sum_over_intermediate_correlation_func(generator):
    inter_corr = reduce(iadd, generator)  # sum over spectra (output has shape len(dspacs) x npulses*nslits)
    # average of inverse intermediate correlation func (Eq.8 in POLDI concept paper)
    with np.errstate(divide="ignore", invalid="ignore"):
        corr = 1 / np.nansum(1 / inter_corr, axis=1)
    return corr


def _autocorr_spec_linear(y, tof_d1Ang, dspacs, offsets, bin_width, progress):
    arrival_time = tof_d1Ang * dspacs + offsets
    itimes = (arrival_time / bin_width) - 0.5  # correct for first time bin center at bin_width / 2
    ibins = itimes.astype(int)  # quicker than np.floor
    frac_bin = ibins + 1 - itimes
    progress.report()
    return frac_bin * np.take(y, ibins, mode="wrap") + (1 - frac_bin) * np.take(y, ibins + 1, mode="wrap")


def _autocorr_spec_nearest(y, tof_d1Ang, dspacs, offsets, bin_width, progress):
    arrival_time = tof_d1Ang * dspacs + offsets
    # round to nearest bin - note this truncation is not equivalent to floor as
    # implicitly subtracting -0.5 as first time bin center at bin_width / 2 then
    # add 0.5 so that truncation results in rounding of input
    ibins = (arrival_time / bin_width).astype(int)
    progress.report()
    return np.take(y, ibins, mode="wrap")


AlgorithmFactory.subscribe(PoldiAutoCorrelation)
