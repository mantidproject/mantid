# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import numpy as np

from isis_powder.abstract_inst import AbstractInst
from isis_powder.routines import absorb_corrections, common, instrument_settings
from isis_powder.hrpd_routines import hrpd_advanced_config, hrpd_algs, hrpd_param_mapping

import mantid.simpleapi as mantid
from mantid.api import MultiDomainFunction, FunctionFactory
from mantid.fitfunctions import FunctionWrapper
from mantid.kernel import UnitConversion, DeltaEModeType

# A bug on the instrument when recording historic NeXus files (< 2015) caused
# corrupted data. Use raw files for now until sufficient time has past and old
# data is unlikely to be reanalysed.
RAW_DATA_EXT = ".raw"

# Constants
PROMPT_PULSE_INTERVAL = 20000.0
PROMPT_PULSE_RIGHT_WIDTH = 140.0
PROMPT_PULSE_LEFT_WIDTH = 30.0


class HRPD(AbstractInst):
    def __init__(self, **kwargs):
        self._inst_settings = instrument_settings.InstrumentSettings(
            param_map=hrpd_param_mapping.attr_mapping, kwargs=kwargs, adv_conf_dict=hrpd_advanced_config.get_all_adv_variables()
        )

        super(HRPD, self).__init__(
            user_name=self._inst_settings.user_name,
            calibration_dir=self._inst_settings.calibration_dir,
            output_dir=self._inst_settings.output_dir,
            inst_prefix="HRPD",
        )

        self._cached_run_details = {}
        self._sample_details = None

    def focus(self, **kwargs):
        self._switch_tof_window_inst_settings(kwargs.get("window"))
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(
            run_number_string=self._inst_settings.run_number,
            do_van_normalisation=self._inst_settings.do_van_norm,
            do_absorb_corrections=self._inst_settings.do_absorb_corrections,
        )

    def create_vanadium(self, **kwargs):
        self._switch_tof_window_inst_settings(kwargs.get("window"))
        self._inst_settings.update_attributes(kwargs=kwargs)

        return self._create_vanadium(
            run_number_string=self._inst_settings.run_in_range, do_absorb_corrections=self._inst_settings.do_absorb_corrections
        )

    def set_sample_details(self, **kwargs):
        kwarg_name = "sample"
        sample_details_obj = common.dictionary_key_helper(
            dictionary=kwargs,
            key=kwarg_name,
            exception_msg="The argument containing sample details was not found. Please set the following argument: {}".format(kwarg_name),
        )
        self._sample_details = sample_details_obj

    def mask_prompt_pulses_if_necessary(self, ws_list):
        for ws in ws_list:
            if self._inst_settings.fit_prompt_pulse:
                self._subtract_prompt_pulses(ws)
            else:
                self._mask_prompt_pulses(ws)

    def should_subtract_empty_inst(self):
        return self._inst_settings.subtract_empty_inst

    def should_subtract_empty_inst_from_vanadium(self):
        return self.should_subtract_empty_inst()  # HRPD want to treat vanadium same as sample

    def create_solid_angle_corrections(self, vanadium, run_details):
        """
        Creates the solid angle corrections from a vanadium run, only applicable on HRPD otherwise return None
        :param vanadium: The vanadium used to create this
        :param run_details: the run details of to use
        """
        settings = self._inst_settings
        if not settings.do_solid_angle:
            return
        solid_angle = mantid.SolidAngle(InputWorkspace=vanadium)
        solid_angle = mantid.Scale(InputWorkspace=solid_angle, Factor=100, Operation="Multiply")

        eff = mantid.Divide(LHSWorkspace=vanadium, RHSWorkspace=solid_angle)
        eff = mantid.ConvertUnits(InputWorkspace=eff, Target="Wavelength")
        integration_range = settings.eff_integration_range
        # use full range if no range is supplied
        integration_range = integration_range if integration_range is not None else (None, None)
        eff = mantid.Integration(InputWorkspace=eff, RangeLower=integration_range[0], RangeUpper=integration_range[1])

        correction = mantid.Multiply(LHSWorkspace=solid_angle, RHSWorkspace=eff)
        correction = mantid.Scale(InputWorkspace=correction, Factor=1e-5, Operation="Multiply")
        name = "sac" + common.generate_splined_name(run_details.run_number, [])
        path = run_details.van_paths

        mantid.SaveNexus(InputWorkspace=correction, Filename=os.path.join(path, name))
        common.remove_intermediate_workspace(eff)
        common.remove_intermediate_workspace(correction)

    def get_solid_angle_corrections(self, vanadium, run_details):
        if not self._inst_settings.do_solid_angle:
            return
        name = "sac" + common.generate_splined_name(vanadium, [])
        path = run_details.van_paths
        try:
            solid_angle = mantid.Load(Filename=os.path.join(path, name))
            return solid_angle
        except ValueError:
            raise RuntimeError(
                "Could not find " + os.path.join(path, name) + " please run create_vanadium with " '"do_solid_angle_corrections=True"'
            )

    def _generate_input_file_name(self, run_number, file_ext=""):
        """
        Generates a name which Mantid uses within Load to find the file.
        :param run_number: The run number to convert into a valid format for Mantid
        :param file_ext: An optional file extension to add to force a particular format
        :return: A filename that will allow Mantid to find the correct run for that instrument.
        """
        if not file_ext:
            file_ext = RAW_DATA_EXT
        return self._generate_inst_filename(run_number=run_number, file_ext=file_ext)

    def _apply_absorb_corrections(self, run_details, ws_to_correct):
        if self._is_vanadium:
            return hrpd_algs.calculate_van_absorb_corrections(
                ws_to_correct=ws_to_correct,
                multiple_scattering=self._inst_settings.multiple_scattering,
                msevents=self._inst_settings.mayers_mult_scat_events,
            )
        elif self._check_sample_details() and self._sample_details.shape_type() == "slab":
            return hrpd_algs.calculate_slab_absorb_corrections(ws_to_correct=ws_to_correct, sample_details_obj=self._sample_details)
        else:
            return absorb_corrections.run_cylinder_absorb_corrections(
                ws_to_correct=ws_to_correct,
                multiple_scattering=self._inst_settings.multiple_scattering,
                sample_details_obj=self._sample_details,
                is_vanadium=self._is_vanadium,
                msevents=self._inst_settings.mayers_mult_scat_events,
            )

    def _crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_using_crop_list(focused_banks, self._inst_settings.tof_cropping_values)

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        return common.crop_in_tof(
            ws_to_crop=van_ws_to_crop, x_min=self._inst_settings.van_tof_cropping[0], x_max=self._inst_settings.van_tof_cropping[-1]
        )

    def _get_instrument_bin_widths(self):
        return self._inst_settings.focused_bin_widths

    def _get_run_details(self, run_number_string):
        run_number_string_key = self._generate_run_details_fingerprint(run_number_string, self._inst_settings.file_extension)

        if run_number_string_key in self._cached_run_details:
            return self._cached_run_details[run_number_string_key]

        self._cached_run_details[run_number_string_key] = hrpd_algs.get_run_details(
            run_number_string=run_number_string, inst_settings=self._inst_settings, is_vanadium=self._is_vanadium
        )

        return self._cached_run_details[run_number_string_key]

    def _mask_prompt_pulses(self, ws, ispecs=None):
        """
        HRPD has a long flight path from the moderator resulting
        in sharp peaks from the proton pulse that maintain their
        sharp resolution. Here we mask these pulses out that occur
        at 20ms intervals.

        :param ws: The workspace containing the pulses. It is masked in place.
        :param ispecs: list of spectra to mask
        """
        # The number of pulse can vary depending on the data range
        # Compute number of pulses that occur at each 20ms interval.
        x_data = ws.readX(0)
        pulse_min = int(round(x_data[0]) / PROMPT_PULSE_INTERVAL) + 1
        pulse_max = int(round(x_data[-1]) / PROMPT_PULSE_INTERVAL) + 1

        mask_kwargs = {"InputWorkspaceIndexSet": ispecs} if ispecs is not None else {}
        for i in range(pulse_min, pulse_max):
            centre = PROMPT_PULSE_INTERVAL * float(i)
            mantid.MaskBins(
                InputWorkspace=ws,
                OutputWorkspace=ws,
                XMin=centre - PROMPT_PULSE_LEFT_WIDTH,
                XMax=centre + PROMPT_PULSE_RIGHT_WIDTH,
                **mask_kwargs,
            )

    def _subtract_prompt_pulses(self, ws):
        """
        HRPD has a long flight path from the moderator resulting
        in sharp peaks from the proton pulse that maintain their
        sharp resolution. Here we fit these peaks and subtract
        them from the backscattering bank spectra. For the other
        banks (and spectra for which the fit failed) the peaks
         are masked in TOF instead.

        :param ws: The workspace containing the pulses. It is masked in place.
        :param tof_res: fractional dTOF/TOF resolution used to mask Bragg peaks that overlap prompt pulses
        """
        mantid.ConvertToDistribution(Workspace=ws, EnableLogging=False)  # so all pulses can be described by same profile
        ispec_max = 60  # last spectrum index in backscattering detector bank
        dpks_bragg, npulses = self._find_bragg_peak_dspacings_and_prompt_pulses(ws, ispec_end=ispec_max)
        fit_kwargs = {
            "CostFunction": "Unweighted least squares",
            "Minimizer": "Simplex",
            "MaxIterations": 6000,
            "IgnoreInvalidData": True,
            "CreateOutput": False,
            "EnableLogging": False,
            "StoreInADS": False,
        }
        # for now only attempt backscattering detectors
        ispec_failed = list(range(ispec_max + 1, ws.getNumberHistograms()))
        si = ws.spectrumInfo()
        for ispec in range(0, ispec_max + 1):
            func = MultiDomainFunction()
            for ipulse, npulse in enumerate(npulses):
                cen = npulse * PROMPT_PULSE_INTERVAL
                xlo = cen - PROMPT_PULSE_LEFT_WIDTH - 10  # add extra to ensure get background on both sides of peak
                xhi = cen + PROMPT_PULSE_RIGHT_WIDTH + 10
                comp_func = self._setup_single_prompt_pulse_function(ws, ispec, cen, xlo, xhi)
                func.add(comp_func)
                func.setDomainIndex(ipulse, ipulse)
                key_suffix = f"_{ipulse}" if ipulse > 0 else ""
                fit_kwargs["InputWorkspace" + key_suffix] = ws.name()
                fit_kwargs["StartX" + key_suffix] = xlo
                fit_kwargs["EndX" + key_suffix] = xhi
                fit_kwargs["WorkspaceIndex" + key_suffix] = ispec
                # exclude x-ranges where Bragg peaks overlap prompt pulse within some fractional resolution
                exclude = self._get_tofs_to_exclude(si, ispec, dpks_bragg, xlo, xhi)
                if exclude:
                    func.fixParameter(f"f{ipulse}.f1.A0")  # fix constant background - Bragg peak can mess with bg
                    fit_kwargs["Exclude" + key_suffix] = exclude
            # check that at least one fit region has no overlapping prompt pulse
            excluded_keys = [key for key in fit_kwargs.keys() if "Exclude" in key]
            if len(excluded_keys) < len(npulses):
                self._add_ties_to_multidomain_prompt_pulse_func(func)
                fit_output = mantid.Fit(Function=func, **fit_kwargs)
                # subtract prompt pulse only from spectrum
                is_success = fit_output.OutputStatus == "success"
                is_small_change = "Changes in function value are too small" in fit_output.OutputStatus
                if is_success or is_small_change:
                    func = fit_output.Function.function
                    self._free_ties_of_multidomain_prompt_pulse_func(func, fit_kwargs)
                    fit_output_final = mantid.Fit(Function=func, **fit_kwargs)
                    success_final = (
                        fit_output_final.OutputStatus == "success"
                        or "Changes in function value are too small" in fit_output_final.OutputStatus
                    )
                    if success_final:
                        func = fit_output.Function.function
                    y_pulses = self._eval_fitted_prompt_pulse_peaks_only(ws, func)
                    y_nopulse = ws.readY(ispec) - y_pulses
                    y_nopulse[y_nopulse < 0] = 0  # can't have negative counts
                    ws.setY(ispec, y_nopulse)
                else:
                    ispec_failed.append(ispec)
            else:
                ispec_failed.append(ispec)
        mantid.ConvertFromDistribution(Workspace=ws, EnableLogging=False)
        # Mask other banks and spectra for which fit failed
        self._mask_prompt_pulses(ws, ispec_failed)

    @staticmethod
    def _eval_fitted_prompt_pulse_peaks_only(ws, func):
        # accumulate peak functions only (first function in individual composite functions)
        pk_func = FunctionWrapper(func[0][0])
        for ifunc in range(1, func.nDomains()):
            pk_func = pk_func + FunctionWrapper(func[ifunc][0])
        ws_eval = mantid.EvaluateFunction(InputWorkspace=ws, Function=pk_func, EnableLogging=False, StoreInADS=False)
        return ws_eval.readY(1)

    @staticmethod
    def _free_ties_of_multidomain_prompt_pulse_func(func, fit_kwargs):
        for ipulse in range(func.nDomains()):
            func.removeTie(f"f{ipulse}.f0.Centre")
            key_suffix = f"_{ipulse}" if ipulse > 0 else ""
            if "Exclude" + key_suffix in fit_kwargs:
                func.fixParameter(f"f{ipulse}.f0.Centre")  # fix centre as can't fit independently with bragg
            else:
                fitted_cen = func[ipulse][0]["Centre"]
                func[ipulse][0].addConstraints(f"{fitted_cen - 5}<Centre<{fitted_cen + 5}")  # free centre but stricter constraint

    @staticmethod
    def _add_ties_to_multidomain_prompt_pulse_func(func):
        # tie peak parameters to be common for all prompt pulses (tie to last peak func - seems to give better results than first)
        for idomain in range(func.nDomains() - 1):
            for param_name in ["Intensity", "Sigma", "Exponent", "Skew", "Centre"]:
                func.tie(f"f{idomain}.f0.{param_name}", f"f{func.nDomains() - 1}.f0.{param_name}")  # tie to first
        # fix some peak parameters
        for param_name in ["Sigma", "Exponent", "Skew"]:
            func.fixParameter(f"f{func.nDomains() - 1}.f0.{param_name}")

    @staticmethod
    def _get_tofs_to_exclude(specinfo, ispec, dpks_bragg, xlo, xhi, tof_res=0.002):
        exclude = []
        diff_consts = specinfo.diffractometerConstants(ispec)
        for dpk in dpks_bragg:
            tof_pk = UnitConversion.run("dSpacing", "TOF", dpk, 0, DeltaEModeType.Elastic, diff_consts)
            tof_pk_lo = tof_pk * (1 - tof_res)
            tof_pk_hi = tof_pk * (1 + tof_res)
            if xlo < tof_pk_lo < xhi or xlo < tof_pk_hi < xhi or (tof_pk_lo < xlo and tof_pk_hi > xhi):
                exclude.extend([tof_pk_lo, tof_pk_hi])
        return exclude

    @staticmethod
    def _setup_single_prompt_pulse_function(ws, ispec, cen, xlo, xhi):
        comp_func = FunctionFactory.createInitialized(
            "name=PearsonIV, Centre=8, Intensity=1,Sigma=8.5, Exponent=1.5, Skew=-5,"
            "constraints=(0.2<Sigma,1.5<Exponent);name=FlatBackground, A0=0,constraints=(0<A0)"
        )
        comp_func[0].setAttributeValue("CentreShift", cen)
        comp_func[0].setHeight(ws.readY(ispec)[ws.yIndexOfX(cen)])
        comp_func[0].addConstraints(f"{-15}<Centre<{15}")
        comp_func[0].addConstraints(f"{0}<Intensity")
        comp_func[1]["A0"] = min(ws.readY(ispec)[ws.yIndexOfX(xlo)], ws.readY(ispec)[ws.yIndexOfX(xhi)])
        return comp_func

    @staticmethod
    def _find_bragg_peak_dspacings_and_prompt_pulses(ws, ispec_start=0, ispec_end=60, dspac_res=0.001):
        """
        Find d-spacing of Bragg peaks present in workspace
        :param ws: workspace in which to find bragg peaks
        :param ispec_start: start workspace index to look for peaks
        :param ispec_end: stop workspace index to look for peaks (only includes backscattering bank by default)
        :param dspac_res: delta(d)/d resolution to determine whether peaks are equivalent in different spectra
        :return: dpks_bragg: array of avg. d-spacing of Bragg peaks
        :return: npulses: array of prompt pulses observed
        """

        out = mantid.FindPeaksConvolve(
            InputWorkspace=ws,
            StartWorkspaceIndex=ispec_start,
            EndWorkspaceIndex=ispec_end,
            EstimatedPeakExtentNBins=30,
            IOverSigmaThreshold=3.5,
            MergeNearbyPeaks=False,
            FindHighestDataPointInPeak=False,
            CreateIntermediateWorkspaces=False,
            EnableLogging=False,
        )  # note StoreInADS=False still stores individual ws in group in ADS
        peak_cens = out[0]  # table workspace with row per spectrum and col per peak

        npulse_min = int(np.ceil(ws.getXDimension().getMinimum()) // PROMPT_PULSE_INTERVAL)
        npulse_max = int(np.ceil(ws.getXDimension().getMaximum()) // PROMPT_PULSE_INTERVAL)

        si = ws.spectrumInfo()
        dpks = []
        npulses = []
        for ispec in range(out[0].rowCount()):
            diff_consts = si.diffractometerConstants(ispec)
            for icol in range(1, peak_cens.columnCount()):
                tof_pk = peak_cens.cell(ispec, icol)
                # check not NaN (happens if not all spectra have same number of peaks)
                if np.isfinite(tof_pk):
                    overlaps_npulse = [
                        abs(tof_pk - npulse * PROMPT_PULSE_INTERVAL) < PROMPT_PULSE_LEFT_WIDTH
                        for npulse in range(npulse_min, npulse_max + 1)
                    ]  # incl. npulse_max
                    if np.any(overlaps_npulse):
                        npulses.append(npulse_min + np.flatnonzero(overlaps_npulse))
                    else:
                        # Bragg peak found
                        dpks.append(UnitConversion.run("TOF", "dSpacing", tof_pk, 0, DeltaEModeType.Elastic, diff_consts))

        # find top 4 prompt pulses observed
        npulses, nobserved = np.unique(npulses, return_counts=True)
        npulses = np.sort(npulses[np.argsort(-nobserved)[:4]])  # get 4 most likely prompt pulses

        # categorise found Bragg peaks as the same within resolution
        dpks_bragg = []
        if len(dpks) > 0:
            dpk_prev = dpks[0]
            dpk_avg = dpks[0]
            npks_in_cluster = 1
            for dpk in dpks[1:]:
                if abs(dpk - dpk_prev) < dspac_res * dpk_prev:
                    dpk_avg += dpk
                    npks_in_cluster += 1
                else:
                    # end of peak - take avg and start new one
                    dpk_avg /= npks_in_cluster
                    dpks_bragg.append(dpk_avg)
                    dpk_avg = dpk
                    npks_in_cluster = 1
                dpk_prev = dpk
            # end last peak
            dpk_avg /= npks_in_cluster
            dpks_bragg.append(dpk_avg)
        mantid.DeleteWorkspace(out, EnableLogging=False)
        return dpks_bragg, npulses

    def _switch_tof_window_inst_settings(self, tof_window):
        self._inst_settings.update_attributes(advanced_config=hrpd_advanced_config.get_tof_window_dict(tof_window=tof_window))
