# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init
from systemtesting import MantidSystemTest
from mantid.simpleapi import mtd
from mantid.simpleapi import CreateEmptyTableWorkspace
from mantid.simpleapi import LoadEmptyInstrument
from mantid.simpleapi import ConvertToEventWorkspace
from mantid.simpleapi import CompareWorkspaces
from mantid.simpleapi import CorelliPowderCalibrationApply


class CorelliPowderCalibrationApply(MantidSystemTest):
    """
    Build an empty/reference CORELLI instrument and a calibration
    table to test the apply functionality.
    """
    def generate_calitab(self):
        """Generate the calibration table with correct format"""
        self.cali_table_name = "corelli_pd_cali_apl_tab"
        CreateEmptyTableWorkspace(OutputWorkspace=self.cali_table_name)
        _calitab = mdt[self.cali_table_name]
        # fix headers
        headers = [
            "ComponentName",
            "Xposition",
            "Yposition",
            "Zposition",
            "XdirectionCosine",
            "YdirectionCosine",
            "ZdirectionCosine",
            "RotationAngle",
        ]
        datatypes = ["str"] + ["double"] * 7
        for dt, hd in zip(datatypes, headers):
            _calitab.addColumn(dt, hd)
        # insert rows (components)
        _cpts = [["moderator", 0, 0, -19.9997, 0, 0, 0, 0],
                 ["sample-position", 0, 0, 0, 0, 0, 0, 0],
                 [
                     "bank7/sixteenpack", 2.25637, -0.814864, -0.883485, -0.0244456, -0.99953,
                     -0.0184843, 69.4926
                 ],
                 [
                     "bank42/sixteenpack", 2.58643, 0.0725628, 0.0868798, -0.011362, -0.999935,
                     -0.000173303, 91.8796
                 ],
                 [
                     "bank57/sixteenpack", 0.4545, 0.0788326, 2.53234, -0.0158497, -0.999694,
                     0.0189818, 169.519
                 ]]
        for cp in _cpts:
            _calitab.addRow(cp)

    def runTest(self):
        # apply the calibration to a reference CORELLI instrument
        self.ws_reference = "ws_ref"
        self.ws_calbrated = "ws_cal"
        LoadEmptyInstrument(Filename="CORELLI_Definition.xml", OutputWorkspace=self.ws_reference)
        ConvertToEventWorkspace(InputWorkspace=self.ws_reference, OutputWorkspace=self.ws_calbrated)
        self.generate_calitab()
        CorelliPowderCalibrationApply(Workspace=self.ws_calbrated,
                                      CalibrationTable=self.cali_table_name)
        # ensure that the calibration application take place
        _ws_ref = mtd[self.ws_reference]
        _ws_cal = mtd[self.ws_calbrated]
        rst, msg = CompareWorkspaces(_ws_ref, _ws_cal)
        if rst is True:
            raise ValueError("The Calibration is not applied to workspace")

        # check if each component is in the correct location
        # 372739 moderator
        # 372740 sample-position
        # 372870 bank7
        # 373501 bank42
        # 373771 bank57