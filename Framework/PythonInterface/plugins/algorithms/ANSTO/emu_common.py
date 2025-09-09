# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import math
import numpy as np

from typing import Dict, Any

from mantid import mtd

from ansto_common import (
    HdfKey,
    extract_hdf_params,
    FilterPixelsTubes,
    IniParameters,
    IntOption,
    StrOption,
    RangeOption,
)

# some constants
DOPPLER_AMPLITUDE_TOL = 0.001
DOPPLER_SPEED_TOL = 0.1


# the following components are shared by the inelastic and elastic EMU reduction algorithms
class DopplerSupport:
    _frequency: float
    _amplitude: float
    _speed: float
    _phase: float
    _frequency_option: str
    _phase_option: str
    _table: str

    def __init__(self, frequency_option: str, phase_option: str, table: str) -> None:
        self._frequency_option = frequency_option
        self._phase_option = phase_option
        self._table = table

    @property
    def frequency(self) -> float:
        return self._frequency

    @property
    def amplitude(self) -> float:
        return self._amplitude

    @property
    def speed(self) -> float:
        return self._speed

    @property
    def phase(self) -> float:
        return self._phase

    def set_amplitude_speed(self, sample_file: str, ds_index: int = 0) -> None:
        tags = [
            HdfKey("dopp_amp", "/entry1/instrument/doppler/ctrl/amplitude", None),
            HdfKey("dopp_vel", "/entry1/instrument/doppler/ctrl/velocity", None),
        ]
        values, _ = extract_hdf_params(sample_file, tags)
        # mm to metres
        self._amplitude = float(values["dopp_amp"][ds_index]) * 0.001
        self._speed = float(values["dopp_vel"][ds_index])
        self._complete_doppler_params()

    def check_doppler_settings(self, ws_tag: str) -> None:
        # confirm that the doppler amplitude and speed match that of the sample
        mrun = mtd[ws_tag].getRun()
        amp = mrun.getProperty("DopplerAmplitude").value[0]
        spd = mrun.getProperty("DopplerVelocity").value[0]
        if math.fabs(self._amplitude - amp) > DOPPLER_AMPLITUDE_TOL or math.fabs(self._speed - spd) > DOPPLER_SPEED_TOL:
            raise ValueError("Doppler parameter")

    def _complete_doppler_params(self) -> None:
        # performs a matrix inversion of the doppler table to create a linear regression
        try:
            rows = self._table.split(";")
            table = []
            for row in rows:
                table.append([float(x) for x in row.split(",")])
            table = np.array(table, dtype=float)
        except ValueError:
            raise RuntimeError("Invalid Doppler table entries in file.")

        # the fitting works by finding the closest entry in the table
        # that matches the (speed, amp) if either phase or frequency is 'fixed'
        # if the match is not close raise an exception
        # note amp is in mm in table
        speed, amp = self._speed, self._amplitude * 1000.0
        min_ix = None
        if self._frequency_option.lower() == "fixed" or self._phase_option.lower() == "fixed":
            # find the closest match
            distance = np.linalg.norm(table[:, :2] - [speed, amp], axis=1)
            min_ix = np.argmin(distance)
            if distance[min_ix] > 0.01:
                raise RuntimeError("Cannot find suitable doppler entry for {}, {}.".format(speed, amp))

        if min_ix is not None and self._frequency_option.lower() == "fixed":
            freq = table[min_ix, 2]
        elif self._frequency_option.lower() == "auto":
            freq = 0.5 * speed / (math.pi * self._amplitude)
        else:
            freq = float(self._frequency_option)

        if min_ix is not None and self._phase_option.lower() == "fixed":
            phase = table[min_ix, 3]
        else:
            phase = float(self._phase_option)

        self._frequency = freq
        self._phase = phase


class FilterEmuPixelsTubes(FilterPixelsTubes):
    _doppler_window: str

    def __init__(
        self,
        valid_tubes: StrOption,
        valid_pixels: RangeOption,
        pixels_per_tube: int,
        pixel_offset: int,
        doppler_window: str,
    ) -> None:
        FilterPixelsTubes.__init__(self, valid_tubes, valid_pixels, pixels_per_tube, pixel_offset)
        self._doppler_window = doppler_window

    def set(
        self,
        valid_tubes: StrOption = None,
        valid_pixels: RangeOption = None,
        pixels_per_tube: IntOption = None,
        pixel_offset: IntOption = None,
        doppler_window: StrOption = None,
    ) -> None:
        FilterPixelsTubes.set(self, valid_tubes, valid_pixels, pixels_per_tube, pixel_offset)
        if doppler_window is not None:
            self._doppler_window = doppler_window

    def _scan_spectra(self, ws_tag: str) -> None:
        # if the doppler window is not none
        # for each spectrum get the event list
        #   - convert the pulse time to doppler time
        #   - set the mask by the sign of the doppler posn
        # the include is merged with this code to avoid a repetitive
        # scan over the histograms
        # TODO get the start time, frequency and doppler phase for the run
        event_ws = mtd[ws_tag]
        if self._doppler_window:
            pve_drive = self._doppler_window == "pos"
            mrun = event_ws.getRun()
            start_run = mrun.startTime().totalNanoseconds()
            dopp_freq = mrun.getProperty("DopplerFrequency").value[0]
            frame_period = 1.0 / dopp_freq
            phase = mrun.getProperty("DopplerPhase").value[0] * math.pi / 180.0
            omega = 2 * math.pi * dopp_freq
            for i in range(self._nhist):
                evl = event_ws.getSpectrum(i)
                if not self._include[i]:
                    evl.clear(False)
                elif evl.getNumberEvents() > 0:
                    # convert to doppler time in seconds
                    # subtract start time, fmod to period
                    pulses = np.array([x.totalNanoseconds() for x in evl.getPulseTimes()])
                    pulses -= start_run
                    dopp_time = pulses * 1.0e-9
                    dopp_time = np.fmod(dopp_time, frame_period)
                    # get the position of the doppler drive at the pulse time
                    # set the mask accordingly
                    posn = np.sin(omega * dopp_time + phase)
                    mask = (posn > 0.0) if pve_drive else (posn < 0.0)
                    evl.maskCondition(mask)
        else:
            # scan over the spectrum
            for i in range(self._nhist):
                if not self._include[i]:
                    evl = event_ws.getSpectrum(i)
                    evl.clear(False)


class EmuParameters(IniParameters):
    _ev_range: str

    def __init__(self, *args: Any, **kwargs: Any) -> None:
        super().__init__(self, *args, **kwargs)

    def set_energy_transfer_bins(self, doppler: DopplerSupport, efixed: float, analysed_v2: float) -> None:
        self._ev_range = self.get_param(str, "processing", "ev_range", "auto")

        if self._ev_range == "auto":
            # determine max dE and use 0.1ueV steps
            dv = 2 * math.pi * doppler.frequency * doppler.amplitude
            max_dE = 2 * efixed * dv / analysed_v2
            self._ev_range = "{:.3f},{:.5f},{:.3f}".format(-max_dE, 0.0001, max_dE)

    def processing_options(self) -> Dict[str, str]:
        # returns a dict of parameters that are not visible from the UI
        # and affect the processing results
        options = {}
        options["analysis_tubes"] = self._analyse_tubes
        options["integrate_over_peak"] = str(self._cal_peak_intensity)
        options["average_peak_width"] = str(self._average_peak_width)
        options["active_pixels"] = "{}-{}".format(self._pixel_range[0], self._pixel_range[1])
        options["integration_range"] = "{:.3f}-{:.3f}".format(self._integration_range[0], self._integration_range[1])
        options["reset_negatives"] = str(self._reset_negatives)

        return options

    def _load_parameters(self, ini_file: str) -> None:
        self.read(ini_file)

        self._file_extn = self.get_param(str, "processing", "file_extn", ".nx.hdf")

        # complete the doppler parameters first as they are needed for subsequent values
        self._phase_option = self.get_param(str, "processing", "doppler_phase", "fixed")
        self._frequency_option = self.get_param(str, "processing", "doppler_frequency", "fixed")
        def_doppler_table = (
            "2,75,4.244,195.3; 2.4,75,5.093,195.32;"
            "4.7,75,9.962,194.8; 1.2,75,2.546,195.2;"
            "1.67,25,10.644,230.95; 2,25,12.733,230.9; 3,40,11.939,229;"
            "3,75,6.366,195"
        )
        self._doppler_table = self.get_param(str, "processing", "doppler_table", def_doppler_table)

        # default refn tube is the direct closest to beam
        self._analyse_tubes = self.get_param(str, "processing", "analyse_tubes", "16-43")
        self._refn_tubes = self.get_param(str, "processing", "direct_tubes", "72,74,78,84,86,88,90,92,94")

        self._inebackground = self.get_param(float, "processing", "inebackground", 0.0)
        self._totbackground = self.get_param(float, "processing", "totbackground", 0.0)
        self._reset_negatives = self.get_bool("processing", "reset_negatives", False)

        self._q_range = self.get_param(str, "processing", "q_range", "auto")
        self._q2_range = self.get_param(str, "processing", "q2_range", "auto")
        self._2theta_range = self.get_param(str, "processing", "2theta_range", "auto")

        self._search_path = [x.strip() for x in self.get_param(str, "processing", "search_path", "").split(";")]

        self._cal_peak_intensity = self.get_bool("processing", "integrate_over_peak", True)
        self._average_peak_width = self.get_bool("processing", "average_peak_width", True)
        self._integration_range = (
            self.get_param(float, "processing", "integration_range_lo", -0.0015),
            self.get_param(float, "processing", "integration_range_hi", 0.0015),
        )
        self._centre_offset = self.get_param(float, "processing", "centre_offset", -0.00013)

        self._def_y_bins = self.get_param(int, "processing", "def_y_bins", 20)
        pixels = self.get_param(str, "processing", "active_pixels", "0-63").split("-")
        self._pixel_range = (int(pixels[0]), int(pixels[-1]))
        self._doppler_window = self.get_param(str, "processing", "doppler_filter", "")
        self._normalise_spectra = self.get_bool("processing", "normalise_spectra", False)
        self._normalise_incident = self.get_param(str, "processing", "normalise_incident", "BeamMonitor")
