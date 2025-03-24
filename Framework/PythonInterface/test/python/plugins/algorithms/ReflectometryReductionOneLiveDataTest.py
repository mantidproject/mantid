# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import mtd, AlgorithmFactory, AnalysisDataService, DataProcessorAlgorithm
from mantid.kernel import config, Direction, StringListValidator, StringMandatoryValidator
from mantid.simpleapi import CreateWorkspace, ReflectometryReductionOneLiveData, GroupWorkspaces, LoadInstrument
from testhelpers import assertRaisesNothing, create_algorithm


class GetFakeLiveInstrumentValue(DataProcessorAlgorithm):
    """Fake algorithm that simulates getting values from an instrument for known
    block names"""

    def __init__(self):
        super(GetFakeLiveInstrumentValue, self).__init__()
        self._theta_name = "THETA"
        self._s1vg_name = "S1VG"
        self._s2vg_name = "S2VG"

    def PyInit(self):
        self._declare_properties()

    def PyExec(self):
        self._do_execute()

    def _declare_properties(self):
        self.declareProperty(
            name="Instrument",
            defaultValue="",
            direction=Direction.Input,
            validator=StringListValidator(["CRISP", "INTER", "OFFSPEC", "POLREF", "SURF"]),
            doc="Instrument to find live value for.",
        )

        self.declareProperty(
            name="PropertyType",
            defaultValue="Run",
            direction=Direction.Input,
            validator=StringListValidator(["Run", "Block"]),
            doc="The type of property to find",
        )

        self.declareProperty(
            name="PropertyName",
            defaultValue="TITLE",
            direction=Direction.Input,
            validator=StringMandatoryValidator(),
            doc="Name of value to find.",
        )

        self.declareProperty(
            name="Value",
            defaultValue="",
            direction=Direction.Output,
            doc="The live value from the instrument, or an empty string if not found",
        )

    def _do_execute(self):
        propertyName = self.getProperty("PropertyName").value
        if propertyName == self._theta_name:
            self.setProperty("Value", "0.5")
        elif propertyName == self._s1vg_name:
            self.setProperty("Value", "1.001")
        elif propertyName == self._s2vg_name:
            self.setProperty("Value", "0.5")
        else:
            raise RuntimeError("Requested live value for unexpected property name " + propertyName)


AlgorithmFactory.subscribe(GetFakeLiveInstrumentValue)


class GetFakeLiveInstrumentValueAlternativeNames(GetFakeLiveInstrumentValue):
    """Fake algorithm that simulates getting values from an instrument for
    alternative block names"""

    def __init__(self):
        super(GetFakeLiveInstrumentValueAlternativeNames, self).__init__()
        self._theta_name = "Theta"
        self._s1vg_name = "s1vg"
        self._s2vg_name = "s2vg"

    def PyInit(self):
        self._declare_properties()

    def PyExec(self):
        self._do_execute()


AlgorithmFactory.subscribe(GetFakeLiveInstrumentValueAlternativeNames)


class GetFakeLiveInstrumentValuesInvalidNames(GetFakeLiveInstrumentValue):
    """Fake algorithm that simulates a failure when attempting to get values from
    an instrument for invalid block names"""

    def __init__(self):
        super(GetFakeLiveInstrumentValuesInvalidNames, self).__init__()
        self._theta_name = "badTheta"
        self._s1vg_name = "bads1vg"
        self._s2vg_name = "bads2vg"

    def PyInit(self):
        self._declare_properties()

    def PyExec(self):
        self._do_execute()


AlgorithmFactory.subscribe(GetFakeLiveInstrumentValuesInvalidNames)


class GetFakeLiveInstrumentValuesWithZeroTheta(GetFakeLiveInstrumentValue):
    """Fake algorithm that simulates the special case where the instrument is reporting theta as zero"""

    def __init__(self):
        super(GetFakeLiveInstrumentValuesWithZeroTheta, self).__init__()

    def PyInit(self):
        self._declare_properties()

    def PyExec(self):
        propertyName = self.getProperty("PropertyName").value
        if propertyName == self._theta_name:
            self.setProperty("Value", "0.0")
        else:
            self._do_execute()


AlgorithmFactory.subscribe(GetFakeLiveInstrumentValuesWithZeroTheta)


class ReflectometryReductionOneLiveDataTest(unittest.TestCase):
    INPUT_WS_ERROR = "Invalid value for property InputWorkspace"

    def setUp(self):
        self._instrument_name = "INTER"
        self._setup_environment()
        self._setup_workspaces()

    def tearDown(self):
        self._reset_workspaces()
        self._reset_environment()

    def test_basic_reduction_works(self):
        workspace = self._run_algorithm_with_defaults()
        self.assertEqual(workspace.dataX(0).size, 55)
        self._assert_delta(workspace.dataX(0)[0], 0.000523)
        self._assert_delta(workspace.dataX(0)[33], 0.002217)
        self._assert_delta(workspace.dataX(0)[54], 0.005379)
        self._assert_delta(workspace.dataY(0)[4], 0.039447)
        self._assert_delta(workspace.dataY(0)[33], 0.00003)
        self._assert_delta(workspace.dataY(0)[53], 0.0)

    def test_basic_reduction_history(self):
        workspace = self._run_algorithm_with_defaults()
        expected = [
            "CloneWorkspace",
            "GetFakeLiveInstrumentValue",
            "GetFakeLiveInstrumentValue",
            "GetFakeLiveInstrumentValue",
            "AddSampleLogMultiple",
            "LoadInstrument",
            "SetInstrumentParameter",
            "SetInstrumentParameter",
            "ReflectometryISISLoadAndProcess",
        ]
        self._check_history(workspace, expected)

    def test_missing_inputs(self):
        self.assertRaises(TypeError, ReflectometryReductionOneLiveData)

    def test_invalid_input_workspace(self):
        self.assertRaisesRegex(ValueError, self.INPUT_WS_ERROR, ReflectometryReductionOneLiveData, InputWorkspace="bad")

    def test_invalid_output_workspace(self):
        self.assertRaises(RuntimeError, ReflectometryReductionOneLiveData, InputWorkspace=self._input_ws, OutputWorkspace="")

    def test_invalid_property(self):
        self.assertRaises(
            TypeError,
            ReflectometryReductionOneLiveData,
            InputWorkspace=self._input_ws,
            OutputWorkspace="output",
            Instrument=self._instrument_name,
            GetLiveValueAlgorithm="GetFakeLiveInstrumentValue",
            BadProperty="badvalue",
        )

    def test_all_child_properties_are_present(self):
        # Get the properties for the child algorithm, apart from a list of known
        # exclusions
        child_alg = create_algorithm("ReflectometryISISLoadAndProcess")
        excluded = [
            "InputRunList",
            "ThetaIn",
            "ThetaLogName",
            "HideInputWorkspaces",
            "OutputWorkspaceTransmission",
            "OutputWorkspaceFirstTransmission",
            "OutputWorkspaceSecondTransmission",
            "OutputWorkspaceBinned",
            "OutputWorkspaceWavelength",
        ]
        child_props = set([prop.name for prop in child_alg.getProperties() if prop.name not in excluded])
        # Check the child properties exist in the parent algorithm
        actual_alg = create_algorithm("ReflectometryReductionOneLiveData")
        actual_props = set([prop.name for prop in actual_alg.getProperties()])
        if not child_props.issubset(actual_props):
            assert False, "The following child properties are not implemented in the parent algorithm:\n" + str(
                child_props.difference(actual_props)
            )

    def test_instrument_was_set_on_output_workspace(self):
        workspace = self._run_algorithm_with_defaults()
        self.assertEqual(workspace.getInstrument().getName(), self._instrument_name)

        load_inst_history = self._get_child_alg_history(workspace, "LoadInstrument")
        self.assertIsNotNone(load_inst_history)
        self.assertEqual(load_inst_history.getPropertyValue("RewriteSpectraMap"), "False")

    def test_sample_log_values_were_set_on_output_workspace(self):
        workspace = self._run_algorithm_with_defaults()
        self._check_sample_log_values(workspace)

    def test_sample_log_values_were_set_on_output_workspace_for_alternative_block_names(self):
        workspace = self._run_algorithm_with_alternative_names()
        self._check_sample_log_values(workspace)

    def test_sample_log_values_are_not_set_on_input_workspace(self):
        self._run_algorithm_with_defaults()

        log_names = [log.name for log in self._input_ws.getRun().getProperties()]
        self.assertNotIn("THETA", log_names)
        self.assertNotIn("S1VG", log_names)
        self.assertNotIn("S2VG", log_names)

    def test_algorithm_fails_for_invalid_block_names(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Unknown algorithm 'GetFakeLiveInstrumentValueInvalidNames'",
            ReflectometryReductionOneLiveData,
            InputWorkspace=self._input_ws,
            OutputWorkspace="output",
            Instrument=self._instrument_name,
            GetLiveValueAlgorithm="GetFakeLiveInstrumentValueInvalidNames",
        )

    def test_reduction_works_if_theta_is_zero(self):
        workspace = self._run_algorithm_with_zero_theta()
        expected = [
            "CloneWorkspace",
            "GetFakeLiveInstrumentValuesWithZeroTheta",
            "GetFakeLiveInstrumentValuesWithZeroTheta",
            "GetFakeLiveInstrumentValuesWithZeroTheta",
            "AddSampleLogMultiple",
            "LoadInstrument",
            "SetInstrumentParameter",
            "SetInstrumentParameter",
            "ReflectometryISISLoadAndProcess",
        ]
        self._check_history(workspace, expected)

    def test_slits_gaps_are_set_up_on_output_workspace(self):
        workspace = self._run_algorithm_with_defaults()
        slit1vg = workspace.getInstrument().getComponentByName("slit1").getNumberParameter("vertical gap")
        slit2vg = workspace.getInstrument().getComponentByName("slit2").getNumberParameter("vertical gap")
        self.assertEqual(slit1vg[0], 1.001)
        self.assertEqual(slit2vg[0], 0.5)

    def test_workspace_groups_are_handled_correctly(self):
        ws_grp_name = "input_grp"
        self._create_test_workspace_group(["ws1", "ws2"], ws_grp_name)
        self._default_args["InputWorkspace"] = ws_grp_name
        self._run_algorithm_with_defaults()

        # The algorithm should not loop through workspace groups, it should pass them straight through.
        # We only output an IvsLam workspace from the reduction when workspace groups are correctly handled this way.
        self.assertTrue(AnalysisDataService.doesExist("IvsLam"))

    def _setup_environment(self):
        self._old_facility = config["default.facility"]
        if self._old_facility.strip() == "":
            self._old_facility = "TEST_LIVE"
        config.setFacility("ISIS")

        self._old_instrument = config["default.instrument"]
        config["default.instrument"] = self._instrument_name

    def _reset_environment(self):
        config.setFacility(self._old_facility)
        config["default.instrument"] = self._old_instrument

    def _setup_workspaces(self):
        self._input_ws = self._create_test_workspace()
        self._default_args = {
            "InputWorkspace": self._input_ws,
            "OutputWorkspace": "output",
            "Instrument": self._instrument_name,
            "GetLiveValueAlgorithm": "GetFakeLiveInstrumentValue",
        }

    def _reset_workspaces(self):
        mtd.clear()

    def _create_test_workspace_group(self, ws_names, grp_name):
        for ws_name in ws_names:
            self._create_test_workspace(ws_name)
        GroupWorkspaces(InputWorkspaces=ws_names, OutputWorkspace=grp_name)

    def _create_test_workspace(self, ws_name="input_ws"):
        """Create a test workspace with reflectometry data but no instrument or sample logs"""
        nSpec = 4
        x = range(5, 100000, 1000)
        y = [
            6,
            1,
            0,
            0,
            0,
            3,
            22,
            40,
            59,
            135,
            241,
            406,
            645,
            932,
            1096,
            1348,
            1559,
            1640,
            1790,
            1933,
            2072,
            2349,
            2629,
            2838,
            2996,
            3141,
            3170,
            3250,
            3260,
            3359,
            3460,
            3316,
            3335,
            3470,
            3384,
            3367,
            3412,
            3431,
            3541,
            3569,
            3592,
            3526,
            3619,
            3757,
            3840,
            4031,
            4076,
            4342,
            4559,
            4998,
            5414,
            6260,
            8358,
            11083,
            11987,
            10507,
            9165,
            8226,
            6979,
            6068,
            5295,
            4587,
            4005,
            3489,
            3018,
            2728,
            2398,
            2220,
            1963,
            1753,
            1516,
            1384,
            1355,
            1192,
            1153,
            1002,
            944,
            926,
            788,
            778,
            687,
            618,
            586,
            531,
            456,
            305,
            77,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
        ]
        monitor_y = [
            14,
            7,
            583,
            97285,
            1474787,
            3362154,
            4790669,
            6768912,
            9359711,
            10443338,
            10478945,
            9957954,
            9387695,
            8660614,
            8354808,
            8036977,
            7649065,
            7067769,
            6275327,
            5558998,
            4898033,
            4307711,
            3776162,
            3302736,
            2895372,
            2539087,
            2226028,
            1956162,
            1721463,
            1518854,
            1343139,
            1193432,
            1057989,
            942001,
            839505,
            748921,
            669257,
            598804,
            537145,
            482685,
            433356,
            390183,
            352178,
            318584,
            288155,
            261372,
            237360,
            216296,
            197369,
            180357,
            164538,
            151010,
            138316,
            126428,
            116943,
            107688,
            98584,
            90757,
            81054,
            59764,
            21151,
            372,
            18,
            4,
            3,
            1,
            3,
            3,
            0,
            3,
            1,
            0,
            2,
            1,
            1,
            1,
            3,
            1,
            1,
            0,
            1,
            0,
            1,
            1,
            2,
            1,
            0,
            1,
            2,
            2,
            0,
            0,
            0,
            0,
            1,
            0,
            1,
            2,
            0,
        ]
        dataX = list()
        dataY = list()
        for i in range(0, nSpec - 1):
            dataX += x
            dataY += monitor_y
        dataX += x
        dataY += y

        CreateWorkspace(NSpec=nSpec, UnitX="TOF", DataX=dataX, DataY=dataY, OutputWorkspace=ws_name)
        LoadInstrument(Workspace=ws_name, RewriteSpectraMap=True, InstrumentName=self._instrument_name)
        return mtd[ws_name]

    def _run_algorithm_with_defaults(self):
        return self._run_algorithm(self._default_args)

    def _run_algorithm_with_alternative_names(self):
        args = self._default_args
        args["GetLiveValueAlgorithm"] = "GetFakeLiveInstrumentValueAlternativeNames"
        return self._run_algorithm(args)

    def _run_algorithm_with_zero_theta(self):
        args = self._default_args
        args["GetLiveValueAlgorithm"] = "GetFakeLiveInstrumentValuesWithZeroTheta"
        return self._run_algorithm(args)

    def _run_algorithm(self, args):
        alg = create_algorithm("ReflectometryReductionOneLiveData", **args)
        assertRaisesNothing(self, alg.execute)
        return mtd["output"]

    def _assert_delta(self, value1, value2):
        self.assertEqual(round(value1, 6), round(value2, 6))

    def _check_history(self, ws, expected, unroll=True):
        """Return true if algorithm names listed in algorithmNames are found in the
        workspace's history. If unroll is true, checks the child histories, otherwise
        checks the top level history (the latter is required for sliced workspaces where
        the child workspaces have lost their parent's history)
        """
        history = ws.getHistory()
        if unroll:
            reductionHistory = history.getAlgorithmHistory(history.size() - 1)
            algHistories = reductionHistory.getChildHistories()
            algNames = [alg.name() for alg in algHistories]
        else:
            algNames = [alg.name() for alg in history]
        self.assertEqual(algNames, expected)

    @staticmethod
    def _get_child_alg_history(ws, child_name):
        history = ws.getHistory()
        reduction_history = history.getAlgorithmHistory(history.size() - 1)
        for child_history in reduction_history.getChildHistories():
            if child_history.name() == child_name:
                return child_history

        return None

    def _check_sample_log_values(self, workspace):
        expected_logs = {"THETA": (0.5, "deg"), "S1VG": (1.001, "m"), "S2VG": (0.5, "m")}
        actual_logs = workspace.getRun().getProperties()

        matched_names = []
        for log in actual_logs:
            if log.name in expected_logs:
                matched_names.append(log.name)
                expected_log_details = expected_logs[log.name]
                self.assertEqual(log.value, expected_log_details[0])
                self.assertEqual(log.units, expected_log_details[1])

        self.assertEqual(sorted(matched_names), sorted(expected_logs.keys()))


if __name__ == "__main__":
    unittest.main()
