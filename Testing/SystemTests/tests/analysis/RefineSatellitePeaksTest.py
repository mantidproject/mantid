from mantid.api import FileFinder
from mantid.simpleapi import RefineSatellitePeaks, Load
import unittest
import stresstesting


def load_files():
    md_path = FileFinder.getFullPath("WISH_md_small.nxs")
    main_peaks_path = FileFinder.getFullPath("WISH_peak_hkl_small.nxs")
    satellite_peaks_path = FileFinder.getFullPath(
        "WISH_peak_hkl_frac_small.nxs")

    md_workspace = Load(Filename=md_path, OutputWorkspace='md_workspace')
    main_peaks = Load(Filename=main_peaks_path,
                      OutputWorkspace='main_peaks')
    satellite_peaks = Load(Filename=satellite_peaks_path,
                           OutputWorkspace='satellite_peaks')
    return md_workspace, main_peaks, satellite_peaks


class RefineSatellitePeaksTestFixedNumQ(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return ["WISH_md_small.nxs", "WISH_peak_hkl_small.nxs", "WISH_peak_hkl_frac_small.nxs"]

    def runTest(self):
        md_workspace, main_peaks, satellite_peaks = load_files()

        fixed_params = {
            "PeakRadius": 0.3,
            "BackgroundInnerRadius": 0.3,
            "BackgroundOuterRadius": 0.4,
            "OutputWorkspace": "refine_peaks_test"
        }

        k = 2

        self._satellites_refined = RefineSatellitePeaks(MainPeaks=main_peaks, SatellitePeaks=satellite_peaks, MDWorkspace=md_workspace,
                                                        NumOfQs=k, **fixed_params)

    def validate(self):
        return self._satellites_refined.name(),'refine_satellites_fixed_q_test.nxs'


class RefineSatellitePeaksTestAutoFindQ(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return ["WISH_md_small.nxs", "WISH_peak_hkl_small.nxs", "WISH_peak_hkl_frac_small.nxs"]

    def runTest(self):
        md_workspace, main_peaks, satellite_peaks = load_files()

        fixed_params = {
            "PeakRadius": 0.3,
            "BackgroundInnerRadius": 0.3,
            "BackgroundOuterRadius": 0.4,
            "OutputWorkspace": "refine_peaks_test"
        }

        threshold = 1.0
        self._satellites_refined = RefineSatellitePeaks(MainPeaks=main_peaks, SatellitePeaks=satellite_peaks, MDWorkspace=md_workspace,
                                                        ClusterThreshold=threshold, **fixed_params)

    def validate(self):
        return self._satellites_refined.name(),'refine_satellites_auto_q_test.nxs'


if __name__ == "__main__":
    unittest.main()
