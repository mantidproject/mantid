# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import FindSatellitePeaks, Load
import systemtesting
import platform


def load_files():
    md_workspace = Load(Filename="WISH_md_small.nxs", OutputWorkspace="md_workspace")
    main_peaks = Load(Filename="WISH_peak_hkl_small.nxs", OutputWorkspace="main_peaks")
    satellite_peaks = Load(Filename="WISH_peak_hkl_frac_small.nxs", OutputWorkspace="satellite_peaks")
    return md_workspace, main_peaks, satellite_peaks


class FindSatellitePeaksTestFixedNumQ(systemtesting.MantidSystemTest):
    def skipTests(self):
        # Numbers are different on ARM architecture so skip test
        return "arm" in platform.machine()

    def requiredFiles(self):
        return ["WISH_md_small.nxs", "WISH_peak_hkl_small.nxs", "WISH_peak_hkl_frac_small.nxs"]

    def runTest(self):
        md_workspace, main_peaks, satellite_peaks = load_files()

        fixed_params = {
            "PeakRadius": 0.3,
            "BackgroundInnerRadius": 0.3,
            "BackgroundOuterRadius": 0.4,
            "OutputWorkspace": "refine_peaks_test",
        }

        k = 2

        self._satellites_refined = FindSatellitePeaks(
            NuclearPeaks=main_peaks, SatellitePeaks=satellite_peaks, MDWorkspace=md_workspace, NumOfQs=k, **fixed_params
        )

    def validate(self):
        return self._satellites_refined.name(), "refine_satellites_fixed_q_test_04012021.nxs"


class FindSatellitePeaksTestAutoFindQ(systemtesting.MantidSystemTest):
    def skipTests(self):
        # Numbers are different on ARM architecture so skip test
        return "arm" in platform.machine()

    def requiredFiles(self):
        return ["WISH_md_small.nxs", "WISH_peak_hkl_small.nxs", "WISH_peak_hkl_frac_small.nxs"]

    def runTest(self):
        md_workspace, main_peaks, satellite_peaks = load_files()

        fixed_params = {
            "PeakRadius": 0.3,
            "BackgroundInnerRadius": 0.3,
            "BackgroundOuterRadius": 0.4,
            "OutputWorkspace": "refine_peaks_test",
        }

        threshold = 1.0
        self._satellites_refined = FindSatellitePeaks(
            NuclearPeaks=main_peaks, SatellitePeaks=satellite_peaks, MDWorkspace=md_workspace, ClusterThreshold=threshold, **fixed_params
        )

    def validate(self):
        return self._satellites_refined.name(), "refine_satellites_auto_q_test_04012021.nxs"
