# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement, OptionalFloat
import testhelpers
import unittest

import sys

try:
    unicode("test for unicode type")
except NameError:
    unicode = str


class TOFTOFScriptElementTest(unittest.TestCase):
    @staticmethod
    def setMinimumValidInputs(scriptElement):
        scriptElement.reset()
        scriptElement.facility_name = "MLZ"
        scriptElement.instrument_name = "TOFTOF"

        # prefix of (some) workspace names
        scriptElement.prefix = "ws"

        # data files are here
        scriptElement.dataDir = "/Somepath/somewhere/"

        # vanadium runs & comment
        scriptElement.vanRuns = ""
        scriptElement.vanCmnt = ""
        scriptElement.vanTemp = OptionalFloat(None)

        # empty can runs, comment, and factor
        scriptElement.ecRuns = ""
        scriptElement.ecTemp = OptionalFloat(None)
        scriptElement.ecFactor = 1

        # data runs: [(runs,comment, temperature), ...]
        scriptElement.dataRuns = [[unicode("0:5"), unicode("Comment for Run 0:5"), OptionalFloat(None)]]

        # additional parameters
        scriptElement.binEon = False
        scriptElement.binEstart = 0.0
        scriptElement.binEstep = 0.0
        scriptElement.binEend = 0.0

        scriptElement.binQon = False
        scriptElement.binQstart = 0.0
        scriptElement.binQstep = 0.0
        scriptElement.binQend = 0.0

        scriptElement.maskDetectors = ""

        # options
        scriptElement.subtractECVan = False
        scriptElement.normalise = TOFTOFScriptElement.NORM_NONE
        scriptElement.correctTof = TOFTOFScriptElement.CORR_TOF_NONE
        scriptElement.replaceNaNs = False
        scriptElement.createDiff = False
        scriptElement.keepSteps = False

        # save data
        scriptElement.saveDir = ""
        scriptElement.saveSofTWNxspe = False
        scriptElement.saveSofTWNexus = False
        scriptElement.saveSofTWAscii = False
        scriptElement.saveSofQWNexus = False
        scriptElement.saveSofQWAscii = False

    @staticmethod
    def setValidInputs(scriptElement):
        scriptElement.reset()
        scriptElement.facility_name = "MLZ"
        scriptElement.instrument_name = "TOFTOF"

        # prefix of (some) workspace names
        scriptElement.prefix = "ws"

        # data files are here
        scriptElement.dataDir = "/Somepath/somewhere/"

        # vanadium runs & comment
        scriptElement.vanRuns = "1:3"
        scriptElement.vanCmnt = "vanadium comment"
        scriptElement.vanTemp = OptionalFloat(None)

        # empty can runs, comment, and factor
        scriptElement.ecRuns = ""
        scriptElement.ecTemp = OptionalFloat(None)
        scriptElement.ecFactor = 1

        # data runs: [(runs,comment, temperature), ...]
        scriptElement.dataRuns = [[unicode("0:5"), unicode("Comment for Run 0:5"), OptionalFloat(None)]]

        # additional parameters
        scriptElement.binEon = True
        scriptElement.binEstart = 0.0
        scriptElement.binEstep = 0.1
        scriptElement.binEend = 1.0

        scriptElement.binQon = True
        scriptElement.binQstart = 0.0
        scriptElement.binQstep = 0.1
        scriptElement.binQend = 1.0

        scriptElement.maskDetectors = ""

        # options
        scriptElement.subtractECVan = False
        scriptElement.normalise = TOFTOFScriptElement.NORM_NONE
        scriptElement.correctTof = TOFTOFScriptElement.CORR_TOF_NONE
        scriptElement.replaceNaNs = False
        scriptElement.createDiff = False
        scriptElement.keepSteps = False

        # save data
        scriptElement.saveDir = ""
        scriptElement.saveSofTWNxspe = False
        scriptElement.saveSofTWNexus = False
        scriptElement.saveSofTWAscii = False
        scriptElement.saveSofQWNexus = False
        scriptElement.saveSofQWAscii = False

    def setUp(self):
        self.scriptElement = TOFTOFScriptElement()
        self.setValidInputs(self.scriptElement)

    def tearDown(self):
        self.scriptElement = None
        # done

    def test_that_inputs_saved_and_loaded_correctly(self):
        scriptElement2 = TOFTOFScriptElement()
        scriptElement2.from_xml(self.scriptElement.to_xml())
        scriptElement2.facility_name = "MLZ"
        scriptElement2.instrument_name = "TOFTOF"
        for name in dir(self.scriptElement):
            if not name.startswith("__") and not hasattr(getattr(self.scriptElement, name), "__call__"):
                self.assertEqual(getattr(self.scriptElement, name), getattr(scriptElement2, name))

    def test_that_script_is_executable_in_mantid(self):
        def exec_script(script):
            exec(script, dict(), dict())

        # data files are here
        self.scriptElement.dataDir = ""

        # vanadium runs & comment
        self.scriptElement.vanRuns = "TOFTOFTestdata.nxs"

        # empty can runs, comment, and factor
        self.scriptElement.ecRuns = "TOFTOFTestdata.nxs"
        self.scriptElement.ecTemp = OptionalFloat(21.0)
        self.scriptElement.ecFactor = 0.9

        # data runs: [(runs,comment, temperature), ...]
        self.scriptElement.dataRuns = [
            [unicode("TOFTOFTestdata.nxs"), unicode("H2O 21C"), OptionalFloat(None)],
            [unicode("TOFTOFTestdata.nxs"), unicode("H2O 34C"), OptionalFloat(34.0)],
        ]

        self.scriptElement.maskDetectors = "1,2"

        # options
        self.scriptElement.subtractECVan = True
        self.scriptElement.normalise = TOFTOFScriptElement.NORM_MONITOR
        self.scriptElement.correctTof = TOFTOFScriptElement.CORR_TOF_VAN
        self.scriptElement.replaceNaNs = True
        self.scriptElement.createDiff = True
        self.scriptElement.keepSteps = True

        testhelpers.assertRaisesNothing(self, exec_script, self.scriptElement.to_script().replace("import matplotlib.pyplot as plt", ""))

    def test_that_script_has_correct_syntax(self):
        self.scriptElement.binEon = False
        self.scriptElement.binQon = False
        self.scriptElement.dataRuns = [("0:5", "Comment for Run 0:5", None)]

        script = self.scriptElement.to_script()

        HasNullBytesError = TypeError
        if sys.version_info >= (3, 5):
            HasNullBytesError = ValueError
        try:
            compiledScript = compile(script, "<string>", "exec")
        except SyntaxError as e:
            self.fail("generated script has a syntax error: '{}'".format(e))
        except HasNullBytesError as e:
            self.fail("generated script has null bytes: '{}'".format(e))
        self.assertIsNotNone(compiledScript)

    def test_that_inputs_are_validated_correctly(self):
        try:
            self.scriptElement.validate_inputs()
        except RuntimeError as e:
            self.fail(e)

        # some handy aliases:
        OptFloat = OptionalFloat

        # [(inputDict, causesException=True), ...]
        modifiedValues = [
            ({}, False),
            ({"correctTof": TOFTOFScriptElement.CORR_TOF_VAN}, True),
            ({"correctTof": TOFTOFScriptElement.CORR_TOF_VAN, "vanRuns": "0:2", "vanCmnt": "vanComment"}, False),
            ({"subtractECVan": True, "vanRuns": "", "vanCmnt": "vanComment", "ecRuns": "3:5"}, True),
            ({"subtractECVan": True, "vanRuns": "", "vanCmnt": "", "ecRuns": "3:5"}, True),
            ({"subtractECVan": True, "vanRuns": "0:2", "vanCmnt": "vanComment", "ecRuns": ""}, True),
            ({"subtractECVan": True, "vanRuns": "0:2", "vanCmnt": "vanComment", "ecRuns": "3:5"}, False),
            ({}, False),
            ({"binEon": True, "binEstart": 1.0, "binEstep": 0.1, "binEend": 1.0}, True),
            ({"binEon": True, "binEstart": 0.0, "binEstep": 2.0, "binEend": 1.0}, True),
            ({"binEon": True, "binEstart": 0.0, "binEstep": 0.0, "binEend": 1.0}, True),
            ({"binEon": True, "binEstart": 0.0, "binEstep": -0.1, "binEend": 1.0}, True),
            ({"binEon": False, "binEstart": 0.0, "binEstep": -0.1, "binEend": 1.0}, False),
            ({"binQon": True, "binQstart": 1.0, "binQstep": 0.1, "binQend": 1.0}, True),
            ({"binQon": True, "binQstart": 0.0, "binQstep": 2.0, "binQend": 1.0}, True),
            ({"binQon": True, "binQstart": 0.0, "binQstep": 0.0, "binQend": 1.0}, True),
            ({"binQon": True, "binQstart": 0.0, "binQstep": -0.1, "binQend": 1.0}, True),
            ({"binQon": False, "binQstart": 1.0, "binQstep": 0.1, "binQend": 1.0}, False),
            ({"dataRuns": []}, True),
            ({"dataRuns": [[unicode("0:5"), unicode(""), OptFloat(None)]]}, True),
            (
                {
                    "dataRuns": [
                        [unicode("0:5"), unicode("Comment for Run 0:5"), OptFloat(None)],
                        [unicode("6:7"), unicode(""), OptFloat(None)],
                    ]
                },
                True,
            ),
            (
                {
                    "dataRuns": [
                        [unicode("0:5"), unicode(""), OptFloat(None)],
                        [unicode("6:7"), unicode("Comment for Run 6:7"), OptFloat(None)],
                    ]
                },
                True,
            ),
            ({"vanRuns": "0:2", "vanCmnt": ""}, True),
            ({"vanRuns": "0:2", "vanCmnt": "Comment for Vanadium"}, False),
            ({"saveSofTWNxspe": True, "saveSofTWAscii": True, "saveDir": ""}, True),
            ({"saveSofTWNexus": True, "saveSofQWNexus": False, "saveDir": ""}, True),
            ({"saveSofQWNexus": True, "saveSofQWAscii": True, "saveDir": "/some/SaveDir/"}, False),
        ]

        def executeSubTest(inputs, shouldThrow):
            self.setMinimumValidInputs(self.scriptElement)
            for name, value in inputs.items():
                setattr(self.scriptElement, name, value)

            try:
                self.scriptElement.validate_inputs()
            except RuntimeError as e:
                if not shouldThrow:
                    self.fail("Valid input did cause an exception: '{}'.\nThe input was: {}".format(e, inputs))
            else:
                if shouldThrow:
                    self.fail("Invalid input did NOT cause an exception!\nThe input was: {}".format(inputs))

        for inputs, shouldThrow in modifiedValues:
            if hasattr(self, "subTest"):
                with self.subTest():
                    executeSubTest(inputs, shouldThrow)
            else:
                executeSubTest(inputs, shouldThrow)


if __name__ == "__main__":
    unittest.main()
