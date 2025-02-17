# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm, Progress
from mantid.kernel import Direction, FloatBoundedValidator, UnitConversion, DeltaEModeType, UnitParams, UnitParametersMap
import numpy as np
from plugins.algorithms.poldi_utils import (
    get_instrument_settings_from_log,
    get_max_tof_from_chopper,
    get_dspac_limits,
    get_final_dspac_array,
)


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

    def validateInputs(self):
        issues = dict()
        if self.getProperty("WavelengthMax").value <= self.getProperty("WavelengthMin").value:
            issues["WavelengthMax"] = "WavelengthMax must be greater than WavelengthMin."
        return issues

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        lambda_min = self.getProperty("WavelengthMin").value
        lambda_max = self.getProperty("WavelengthMax").value

        cycle_time, slit_offsets, t0_const, l1_chop = get_instrument_settings_from_log(ws)
        # get detector positions from IDF
        si = ws.spectrumInfo()
        tths = np.array([si.twoTheta(ispec) for ispec in range(ws.getNumberHistograms())])
        l2s = [si.l2(ispec) for ispec in range(ws.getNumberHistograms())]
        l1 = si.l1()
        # determine npulses to include in calc
        time_max = get_max_tof_from_chopper(l1, l1_chop, l2s, tths, lambda_max) + slit_offsets[0]
        npulses = int(time_max // cycle_time)
        # get final d-spacing array
        dspac_min, dspac_max = get_dspac_limits(tths.min(), tths.max(), lambda_min, lambda_max)
        bin_width = ws.readX(0)[1] - ws.readX(0)[0]
        dspacs = get_final_dspac_array(bin_width, dspac_min, dspac_max, time_max)
        # perform auto-correlation (Eq. 7 in POLDI concept paper)
        ipulses = np.arange(npulses)[:, None]
        offsets = (ipulses * cycle_time + slit_offsets).flatten()
        inter_corr = np.zeros((len(dspacs), len(offsets)), dtype=float)  # npulses*nslits
        # sum over all spectra
        params = UnitParametersMap()
        nspec = ws.getNumberHistograms()
        progress = Progress(self, start=0.0, end=1.0, nreports=nspec)
        for ispec in range(nspec):
            params[UnitParams.l2] = l2s[ispec]
            params[UnitParams.twoTheta] = tths[ispec]
            # TOF from chopper to detector for wavelength corresponding to d=1Ang
            # equivalent to DIFC * (Ltot - L1_source-chop) / Ltot
            tof_d1Ang = UnitConversion.run("dSpacing", "TOF", 1.0, l1 - l1_chop, DeltaEModeType.Elastic, params)
            arrival_time = tof_d1Ang * dspacs[:, None] + offsets + t0_const
            # detector clock reset for each pulse
            # equivalent to np.mod(arrival_time, cycle_time) but order of magnitude quicker
            time_in_cycle_period = arrival_time - cycle_time * np.floor(arrival_time / cycle_time)
            itimes = time_in_cycle_period / bin_width
            ibins = np.floor(itimes).astype(int)
            ibins_plus = ibins + 1
            ibins_plus[ibins_plus > ws.blocksize() - 1] = 0
            y = ws.readY(ispec)
            inter_corr += (ibins + 1 - itimes) * y[ibins] + (itimes - ibins) * y[ibins_plus]
            progress.report()
        # average of inverse intermediate correlation func (Eq.8 in POLDI concept paper)
        corr = 1 / np.sum(1 / inter_corr, axis=1)
        ws_corr = self.exec_child_alg("CreateWorkspace", DataX=dspacs, DataY=corr, UnitX="dSpacing", YUnitLabel="Intensity (a.u.)")

        self.setProperty("OutputWorkspace", ws_corr)

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props


AlgorithmFactory.subscribe(PoldiAutoCorrelation)
