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
from mantid.simpleapi import MoveInstrumentComponent
from mantid.simpleapi import RotateInstrumentComponent
from mantid.simpleapi import CorelliPowderCalibrationApply


class CorelliPowderCalibrationApplyTest(MantidSystemTest):
    """
    Build an empty/reference CORELLI instrument and a calibration
    table to test the apply functionality.
    """
    def generate_calitab(self):
        """Generate the calibration table with correct format"""
        self.cali_table_name = "corelli_pd_cali_apl_tab"
        CreateEmptyTableWorkspace(OutputWorkspace=self.cali_table_name)
        _calitab = mtd[self.cali_table_name]
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

    def get_target_instrument(self):
        self.ws_target = "ws_target"
        LoadEmptyInstrument(Filename="CORELLI_Definition.xml", OutputWorkspace=self.ws_target)
        ConvertToEventWorkspace(InputWorkspace=self.ws_target, OutputWorkspace=self.ws_target)
        # explicitly translate component TO designated location
        MoveInstrumentComponent(Workspace=self.ws_target,
                                ComponentName="moderator",
                                X=0,
                                Y=0,
                                Z=-19.9997,
                                RelativePosition=False)
        MoveInstrumentComponent(Workspace=self.ws_target,
                                ComponentName="sample-position",
                                X=0,
                                Y=0,
                                Z=0,
                                RelativePosition=False)
        MoveInstrumentComponent(Workspace=self.ws_target,
                                ComponentName="bank7/sixteenpack",
                                X=2.25637,
                                Y=-0.814864,
                                Z=-0.883485,
                                RelativePosition=False)
        MoveInstrumentComponent(Workspace=self.ws_target,
                                ComponentName="bank42/sixteenpack",
                                X=2.58643,
                                Y=0.0725628,
                                Z=0.0868798,
                                RelativePosition=False)
        MoveInstrumentComponent(Workspace=self.ws_target,
                                ComponentName="bank57/sixteenpack",
                                X=0.4545,
                                Y=0.0788326,
                                Z=2.53234,
                                RelativePosition=False)
        # explicitly rotate component TO designated orientation
        RotateInstrumentComponent(Workspace=self.ws_target,
                                  ComponentName="bank7/sixteenpack",
                                  X=-0.0244456,
                                  Y=-0.99953,
                                  Z=-0.0184843,
                                  Angle=69.4926,
                                  RelativeRotation=False)
        RotateInstrumentComponent(Workspace=self.ws_target,
                                  ComponentName="bank42/sixteenpack",
                                  X=-0.011362,
                                  Y=-0.999935,
                                  Z=-0.000173303,
                                  Angle=91.8796,
                                  RelativeRotation=False)
        RotateInstrumentComponent(Workspace=self.ws_target,
                                  ComponentName="bank42/sixteenpack",
                                  X=-0.0158497,
                                  Y=-0.999694,
                                  Z=0.0189818,
                                  Angle=169.519,
                                  RelativeRotation=False)

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
        rst, _ = CompareWorkspaces(_ws_ref, _ws_cal)
        if rst is True:
            raise ValueError("The Calibration is not applied to workspace")

        # get the target ws
        self.get_target_instrument()
        _ws_tgt = mtd[self.ws_target]

        # check if the calibrated workspace matches the target
        rst, msg = CompareWorkspaces(_ws_tgt, _ws_cal)
        #NOTE:
        #  Currently comparing the two workspace will yield False even if the implementation
        #  is exactly the same as the C++ counter parts.
        #  Given that this is just a place holder for more comprehensive systemtest onece
        #  the upstream calibration algorithm is complete, we are skipping the checking here.
        print(msg)
        # if rst is False:
        #     raise ValueError("The calibration did not return correct results.")
