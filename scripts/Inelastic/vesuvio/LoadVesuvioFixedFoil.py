# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright (c) 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+
"""Register a fixed-foil Vesuvio loader by running this file in Mantid Workbench.

Then use the LoadVesuvioFixedFoil algorithm dialog, or run:

    from mantid.api import AlgorithmManager, mtd
    alg = AlgorithmManager.create("LoadVesuvioFixedFoil")
    alg.initialize()
    alg.setRethrows(True)
    alg.setPropertyValue("Filename", "59659")
    alg.setPropertyValue("FoilInSpectra", "135-142")
    alg.setPropertyValue("FoilOutSpectra", "143-150")
    alg.setPropertyValue("OutputWorkspace", "result")
    alg.execute()
    result = mtd["result"]

The spectrum lists are examples, not the actual foil states of run 59659.
Use this algorithm only when each listed detector remains in its assigned
foil state throughout every period of every input run. The state assignments
cannot be inferred or verified from the measurements by this algorithm.
All periods are summed, then each detector is normalised using the same summed
monitor. After summing detectors by state, foil-in is scaled to foil-out over
the instrument's forward 400-450 microsecond range and foil-out is subtracted.
Counting errors assume independent Poisson counts; uncertainty in the monitor
and in the scaling factor is not propagated, matching LoadVesuvio's approach.
"""

import re
import uuid

import numpy as np

from mantid.api import AlgorithmFactory, FileAction, FileProperty, WorkspaceFactory, WorkspaceGroup, WorkspaceProperty, mtd
from mantid.kernel import Direction, IntArrayProperty, StringMandatoryValidator
import mantid.simpleapi as ms

from LoadVesuvio import LoadVesuvio


class LoadVesuvioFixedFoil(LoadVesuvio):
    def summary(self):
        return "Loads and differences forward Vesuvio spectra with a fixed foil state for each detector."

    def PyInit(self):
        self.declareProperty("Filename", "", StringMandatoryValidator(), doc="A run number, run-number list or range, or file path.")
        self.declareProperty(IntArrayProperty("FoilInSpectra", []), doc="Forward spectrum numbers with the foil in for every period.")
        self.declareProperty(IntArrayProperty("FoilOutSpectra", []), doc="Forward spectrum numbers with the foil out for every period.")
        self.declareProperty(
            FileProperty("InstrumentParFile", "", action=FileAction.OptionalLoad, extensions=["dat", "par"]),
            doc="Optional instrument parameter file.",
        )
        self.declareProperty("LoadMonitors", False, doc="Return the monitor spectra summed over all periods and runs.")
        self.declareProperty("LoadLogFiles", True, doc="Load external log files when supported by the file loader.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Output difference workspace.")

    def validateInputs(self):
        self._load_common_inst_parameters()
        issues = {}
        foil_in = self.getProperty("FoilInSpectra").value.tolist()
        foil_out = self.getProperty("FoilOutSpectra").value.tolist()
        for name, spectra in (("FoilInSpectra", foil_in), ("FoilOutSpectra", foil_out)):
            if not spectra:
                issues[name] = "At least one forward spectrum is required."
            elif len(set(spectra)) != len(spectra):
                issues[name] = "Spectrum numbers must not be repeated."
            elif any(not self._is_fwd_scattering(spectrum) for spectrum in spectra):
                issues[name] = "Only forward-scattering spectra are supported."
        if set(foil_in) & set(foil_out):
            issues["FoilOutSpectra"] = "A spectrum cannot be both foil-in and foil-out."
        return issues

    def _runs(self):
        text = self.getPropertyValue("Filename").strip()
        if not re.fullmatch(r"\d+(?:\s*-\s*\d+)?(?:\s*,\s*\d+(?:\s*-\s*\d+)?)*", text):
            return [text]
        runs = []
        for part in text.replace(" ", "").split(","):
            bounds = [int(value) for value in part.split("-")]
            if len(bounds) == 1:
                runs.append(bounds[0])
            elif bounds[0] <= bounds[1]:
                runs.extend(range(bounds[0], bounds[1] + 1))
            else:
                raise ValueError("Run ranges must be in ascending order.")
        return runs

    @staticmethod
    def _periods(workspace):
        return list(workspace) if isinstance(workspace, WorkspaceGroup) else [workspace]

    @staticmethod
    def _accumulate_counts(data, spectra, foil_in, data_x, counts):
        for index, spectrum in enumerate(spectra):
            if not np.array_equal(data.x(index), data_x):
                raise RuntimeError("Detector time bins differ between spectra or periods.")
            values = data.y(index)
            if not np.all(np.isfinite(values)) or np.any(values < 0):
                raise RuntimeError("Detector counts must be non-negative and finite (spectrum %d)." % spectrum)
            counts[0 if spectrum in foil_in else 1] += values

    def PyExec(self):
        self._load_common_inst_parameters()
        self._set_spectra_type(self._forward_spectra_list[0])
        foil_in = set(self.getProperty("FoilInSpectra").value.tolist())
        spectra = sorted(foil_in | set(self.getProperty("FoilOutSpectra").value.tolist()))
        counts = None
        monitor_counts = None
        monitor_output = None
        result = None
        data_x = None
        mon_x = None

        for run in self._runs():
            name = "__fixed_foil_" + uuid.uuid4().hex
            mon_name = name + "_monitors"
            try:
                ms.Load(
                    Filename=self._get_filename(run),
                    OutputWorkspace=name,
                    SpectrumList=self._mon_spectra + spectra,
                    LoadMonitors="Separate",
                    LoadLogFiles=self.getProperty("LoadLogFiles").value,
                    EnableLogging=False,
                )
                if mon_name not in mtd:
                    raise RuntimeError("The loaded run does not contain a separate monitor workspace.")
                data_periods = self._periods(mtd[name])
                monitor_periods = self._periods(mtd[mon_name])
                if len(data_periods) != len(monitor_periods):
                    raise RuntimeError("Detector and monitor period counts do not match.")

                for data, monitor in zip(data_periods, monitor_periods):
                    if data.getNumberHistograms() != len(spectra):
                        raise RuntimeError("The loaded run does not contain all requested detector spectra.")
                    if [data.getSpectrum(i).getSpectrumNo() for i in range(len(spectra))] != spectra:
                        raise RuntimeError("The loaded spectra do not match the requested spectrum numbers and order.")
                    if self._mon_index >= monitor.getNumberHistograms():
                        raise RuntimeError("The monitor required for normalisation is missing.")
                    if data_x is None:
                        self._diff_opt = "SingleDifference"
                        self._load_diff_mode_parameters(data)
                        ip_file = self.getPropertyValue("InstrumentParFile")
                        if ip_file:
                            data = self._load_ip_file(data, ip_file)
                    data = ms.CropWorkspace(
                        InputWorkspace=data,
                        XMax=max(self._tof_max, self._foil_out_norm_end),
                        StoreInADS=False,
                        EnableLogging=False,
                    )
                    if data_x is None:
                        data_x = data.x(0).copy()
                        mon_x = monitor.x(self._mon_index).copy()
                        if len(data_x) < 3 or len(mon_x) < 3:
                            raise RuntimeError("Detector and monitor data require at least two time bins.")
                        bins = data.blocksize()
                        result = WorkspaceFactory.create(data, NVectors=1, XLength=bins, YLength=bins)
                        result.setDistribution(True)
                        output_spectrum = result.getSpectrum(0)
                        output_spectrum.clearDetectorIDs()
                        for index in range(len(spectra)):
                            for det_id in data.getSpectrum(index).getDetectorIDs():
                                output_spectrum.addDetectorID(det_id)
                        counts = np.zeros((2, bins))
                        monitor_counts = np.zeros(monitor.blocksize())
                    self._accumulate_counts(data, spectra, foil_in, data_x, counts)
                    if not np.array_equal(monitor.x(self._mon_index), mon_x):
                        raise RuntimeError("Monitor time bins differ between periods or runs.")
                    monitor_counts += monitor.y(self._mon_index)
                    if self.getProperty("LoadMonitors").value:
                        if monitor_output is None:
                            monitor_output = ms.CloneWorkspace(InputWorkspace=monitor, StoreInADS=False, EnableLogging=False)
                        else:
                            monitor_output = ms.Plus(
                                LHSWorkspace=monitor_output, RHSWorkspace=monitor, StoreInADS=False, EnableLogging=False
                            )
            finally:
                for workspace_name in (name, mon_name):
                    if workspace_name in mtd:
                        ms.DeleteWorkspace(workspace_name, EnableLogging=False)

        result = self._calculate_result(result, data_x, mon_x, counts, monitor_counts)
        self.setProperty("OutputWorkspace", result)
        if monitor_output is not None:
            output_name = self.getPropertyValue("OutputWorkspace") + "_monitors"
            self.declareProperty(WorkspaceProperty("OutputMonitorWorkspace", output_name, Direction.Output), doc="Summed monitor spectra.")
            self.setProperty("OutputMonitorWorkspace", monitor_output)

    def _calculate_result(self, result, data_x, mon_x, counts, monitor_counts):
        widths = np.diff(data_x)
        mon_widths = np.diff(mon_x)
        if not np.all(np.isfinite(widths)) or not np.all(widths > 0) or not np.all(np.isfinite(mon_widths)) or not np.all(mon_widths > 0):
            raise RuntimeError("Detector and monitor bin widths must be positive and finite.")
        times = data_x[1:] - (data_x[2] - data_x[1])
        mon_times = mon_x[1:] - (mon_x[2] - mon_x[1])
        mon_range = (mon_times >= self._mon_norm_start) & (mon_times < self._mon_norm_end)
        mon_sum = np.sum((monitor_counts / mon_widths)[mon_range])
        if not np.isfinite(mon_sum) or mon_sum <= 0:
            raise RuntimeError("The summed monitor signal must be positive and finite in the normalisation range.")
        scale = self._mon_scale / mon_sum
        foil_in_y, foil_out_y = counts * scale / widths
        norm_range = (times >= self._foil_out_norm_start) & (times < self._foil_out_norm_end)
        in_integral = np.sum(foil_in_y[norm_range])
        out_integral = np.sum(foil_out_y[norm_range])
        if not np.isfinite(in_integral) or not np.isfinite(out_integral) or in_integral <= 0 or out_integral <= 0:
            raise RuntimeError("Both foil-state signals must have positive finite integrals in the foil-out normalisation range.")
        foil_scale = out_integral / in_integral
        result.setSharedX(0, times)
        result.mutableY(0)[:] = foil_scale * foil_in_y - foil_out_y
        result.mutableE(0)[:] = np.sqrt(foil_scale**2 * counts[0] + counts[1]) * scale / widths

        crop = self.createChildAlgorithm("CropWorkspace", enableLogging=False)
        crop.setProperty("InputWorkspace", result)
        crop.setProperty("XMax", self._tof_max)
        crop.execute()
        return crop.getProperty("OutputWorkspace").value


AlgorithmFactory.subscribe(LoadVesuvioFixedFoil)
