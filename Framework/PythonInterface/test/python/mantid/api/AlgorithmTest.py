# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import json
from mantid.kernel import (
    FloatArrayProperty,
    IntArrayProperty,
    StringArrayProperty,
    IntArrayMandatoryValidator,
    FloatArrayMandatoryValidator,
    StringArrayMandatoryValidator,
)
from mantid.simpleapi import CreateSampleWorkspace
from mantid.api import AlgorithmID, AlgorithmManager, AlgorithmFactory, FrameworkManagerImpl, PythonAlgorithm, Workspace
from testhelpers import run_algorithm


class _ParamTester(PythonAlgorithm):
    def category(self):
        return "Examples"

    def PyInit(self):
        self.declareProperty(FloatArrayProperty("FloatInput", FloatArrayMandatoryValidator()))
        self.declareProperty(IntArrayProperty("IntInput", IntArrayMandatoryValidator()))
        self.declareProperty(StringArrayProperty("StringInput", StringArrayMandatoryValidator()))

    def PyExec(self):
        pass


class AlgorithmTest(unittest.TestCase):
    _load = None

    def setUp(self):
        FrameworkManagerImpl.Instance()
        self._alg_factory = AlgorithmFactory.Instance()
        self._alg_factory.subscribe(_ParamTester)
        if self._load is None:
            self.__class__._load = AlgorithmManager.createUnmanaged("Load")
            self._load.initialize()

    def test_alg_attrs_are_correct(self):
        self.assertEqual("Load", self._load.name())
        self.assertEqual(1, self._load.version())
        self.assertEqual("DataHandling", self._load.category())
        self.assertEqual(1, len(self._load.categories()))
        self.assertEqual("DataHandling", self._load.categories()[0])
        self.assertEqual("", self._load.helpURL())
        self.assertEqual(["LoadNexus", "LoadRaw", "LoadBBY"], self._load.seeAlso())

    def test_get_unknown_property_raises_error(self):
        self.assertRaises(RuntimeError, self._load.getProperty, "NotAProperty")

    def test_alg_set_valid_prop_succeeds(self):
        self._load.setProperty("Filename", "LOQ48127.raw")

    def test_alg_set_invalid_prop_raises_error(self):
        alg = AlgorithmManager.createUnmanaged("Load")
        alg.initialize()
        args = ("Filename", "nonexistent.txt")
        self.assertRaises(ValueError, alg.setProperty, *args)

    def test_cannot_execute_with_invalid_properties(self):
        alg = AlgorithmManager.createUnmanaged("Load")
        alg.initialize()
        self.assertRaises(RuntimeError, alg.execute)

    def test_execute_succeeds_with_valid_props(self):
        data = [1.5, 2.5, 3.5]
        alg = run_algorithm("CreateWorkspace", DataX=data, DataY=data, NSpec=1, UnitX="Wavelength", child=True)
        self.assertEqual(alg.isExecuted(), True)
        self.assertEqual(alg.isRunning(), False)
        self.assertEqual(alg.getProperty("NSpec").value, 1)
        self.assertEqual(type(alg.getProperty("NSpec").value), int)
        self.assertEqual(alg.getProperty("NSpec").name, "NSpec")
        ws = alg.getProperty("OutputWorkspace").value
        self.assertTrue(ws.getMemorySize() > 0.0)

        as_str = str(alg)
        self.assertEqual(
            as_str,
            '{"name":"CreateWorkspace","properties":{"DataX":[1.5,2.5,3.5],"DataY":[1.5,2.5,3.5],'
            '"OutputWorkspace":"UNUSED_NAME_FOR_CHILD","UnitX":"Wavelength"},"version":1}',
        )

    def test_execute_succeeds_with_unicode_props(self):
        data = [1.5, 2.5, 3.5]
        kwargs = {"child": True}
        unitx = "Wavelength"
        kwargs["UnitX"] = unitx

        alg = run_algorithm("CreateWorkspace", DataX=data, DataY=data, NSpec=1, **kwargs)
        self.assertEqual(alg.isExecuted(), True)
        self.assertEqual(alg.isRunning(), False)
        self.assertEqual(alg.getProperty("NSpec").value, 1)
        self.assertEqual(type(alg.getProperty("NSpec").value), int)
        self.assertEqual(alg.getProperty("NSpec").name, "NSpec")
        ws = alg.getProperty("OutputWorkspace").value
        self.assertTrue(ws.getMemorySize() > 0.0)

        as_str = str(alg)
        self.assertEqual(
            as_str,
            '{"name":"CreateWorkspace","properties":{"DataX":[1.5,2.5,3.5],"DataY":[1.5,2.5,3.5],'
            '"OutputWorkspace":"UNUSED_NAME_FOR_CHILD","UnitX":"Wavelength"},"version":1}',
        )

    def test_execute_succeeds_with_unicode_kwargs(self):
        props = json.loads('{"DryRun":true}')  # this is always unicode
        run_algorithm("Segfault", **props)

    def test_getAlgorithmID_returns_AlgorithmID_object(self):
        alg = AlgorithmManager.createUnmanaged("Load")
        self.assertEqual(AlgorithmID, type(alg.getAlgorithmID()))

    def test_AlgorithmID_compares_by_value(self):
        alg = AlgorithmManager.createUnmanaged("Load")
        id = alg.getAlgorithmID()
        self.assertEqual(id, id)  # equals itself
        alg2 = AlgorithmManager.createUnmanaged("Load")
        id2 = alg2.getAlgorithmID()
        self.assertNotEqual(id2, id)

    def test_cancel_does_nothing_to_executed_algorithm(self):
        data = [1.0]
        alg = run_algorithm("CreateWorkspace", DataX=data, DataY=data, NSpec=1, UnitX="Wavelength", child=True)
        self.assertEqual(alg.isExecuted(), True)
        self.assertEqual(alg.isRunning(), False)
        alg.cancel()
        self.assertEqual(alg.isExecuted(), True)
        self.assertEqual(alg.isRunning(), False)

    def test_createChildAlgorithm_creates_new_algorithm_that_is_set_as_child(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm("Rebin")

        self.assertTrue(child_alg.isChild())

    def test_createChildAlgorithm_respects_keyword_arguments(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        try:
            parent_alg.createChildAlgorithm(name="Rebin", version=1, startProgress=0.5, endProgress=0.9, enableLogging=True)
        except Exception as exc:
            self.fail("Expected createChildAlgorithm not to throw but it did: %s" % (str(exc)))

        # Unknown keyword
        self.assertRaises(
            Exception,
            parent_alg.createChildAlgorithm,
            name="Rebin",
            version=1,
            startProgress=0.5,
            endProgress=0.9,
            enableLogging=True,
            unknownKW=1,
        )

    def test_createChildAlgorithm_with_kwargs(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm("CreateSampleWorkspace", **{"XUnit": "Wavelength"})

        self.assertTrue(child_alg.isChild())
        child_alg.execute()
        ws = child_alg.getProperty("OutputWorkspace").value

        self.assertEqual("Wavelength", ws.getAxis(0).getUnit().unitID())

    def test_createChildAlgorithm_with_named_args(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm("CreateSampleWorkspace", XUnit="Wavelength")

        self.assertTrue(child_alg.isChild())
        child_alg.execute()
        ws = child_alg.getProperty("OutputWorkspace").value

        self.assertEqual("Wavelength", ws.getAxis(0).getUnit().unitID())

    def test_createChildAlgorithm_with_version_and_kwargs(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm("CreateSampleWorkspace", version=1, **{"XUnit": "Wavelength"})

        self.assertTrue(child_alg.isChild())
        child_alg.execute()
        ws = child_alg.getProperty("OutputWorkspace").value

        self.assertEqual("Wavelength", ws.getAxis(0).getUnit().unitID())

    def test_createChildAlgorithm_with_all_args(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm(
            "CreateSampleWorkspace", startProgress=0.0, endProgress=1.0, enableLogging=False, version=1, **{"XUnit": "Wavelength"}
        )

        self.assertTrue(child_alg.isChild())
        child_alg.execute()
        ws = child_alg.getProperty("OutputWorkspace").value

        self.assertEqual("Wavelength", ws.getAxis(0).getUnit().unitID())

    def test_with_workspace_types(self):
        ws = CreateSampleWorkspace(
            Function="User Defined",
            UserDefinedFunction="name=LinearBackground, A0=0.3;name=Gaussian, PeakCentre=5, Height=10, Sigma=0.7",
            NumBanks=1,
            BankPixelWidth=1,
            XMin=0,
            XMax=10,
            BinWidth=0.1,
        )

        # Setup the model, here a Gaussian, to fit to data
        tryCentre = "4"  # A start guess on peak centre
        sigma = "1"  # A start guess on peak width
        height = "8"  # A start guess on peak height
        myFunc = "name=Gaussian, Height=" + height + ", PeakCentre=" + tryCentre + ", Sigma=" + sigma
        args = {"Function": myFunc, "InputWorkspace": ws, "Output": "fit"}
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm("Fit", 0, 0, True, version=1, **args)
        child_alg.execute()
        out_ws = child_alg.getProperty("OutputWorkspace").value
        self.assertIsInstance(out_ws, Workspace)

    def test_createChildAlgorithm_without_name(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        with self.assertRaisesRegex(ValueError, "algorithm name"):
            parent_alg.createChildAlgorithm(startProgress=0.0, endProgress=1.0, enableLogging=False, version=1, **{"XUnit": "Wavelength"})

    def test_createChildAlgorithm_without_parameters(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        with self.assertRaisesRegex(ValueError, "algorithm name"):
            parent_alg.createChildAlgorithm()

    def test_createChildAlgorithm_with_incorrect_types(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        with self.assertRaises(TypeError):
            parent_alg.createChildAlgorithm(
                "CreateSampleWorkspace", startProgress="0.0", endProgress=1.0, enableLogging=False, version=1, **{"XUnit": "Wavelength"}
            )

    def test_createChildAlgorithm_with_mixed_args_and_kwargs(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm(
            "CreateSampleWorkspace", 0.0, 1.0, version=1, enableLogging=False, **{"XUnit": "Wavelength"}
        )
        self.assertTrue(child_alg.isChild())
        child_alg.execute()
        ws = child_alg.getProperty("OutputWorkspace").value

        self.assertEqual("Wavelength", ws.getAxis(0).getUnit().unitID())

    def test_createChildAlgorithm_with_bool_arg(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm("CreateSampleWorkspace", 0.0, 1.0, version=1, enableLogging=False, Random=True)
        self.assertTrue(child_alg.isChild())
        child_alg.execute()
        ws = child_alg.getProperty("OutputWorkspace").value

        self.assertIsNotNone(ws)

    def test_createChildAlgorithm_with_int_arg(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        num_banks = 4
        child_alg = parent_alg.createChildAlgorithm("CreateSampleWorkspace", 0.0, 1.0, version=1, enableLogging=False, NumBanks=num_banks)

        self.assertTrue(child_alg.isChild())
        child_alg.execute()
        ws = child_alg.getProperty("OutputWorkspace").value

        default_num_histos = 100  # From createSampleWorkspace
        self.assertEqual((default_num_histos * num_banks), ws.getNumberHistograms())

    def test_createChildAlgorithm_with_float_arg(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        child_alg = parent_alg.createChildAlgorithm("CreateSampleWorkspace", 0.0, 1.0, version=1, enableLogging=False, XMin=2.0)
        self.assertTrue(child_alg.isChild())
        child_alg.execute()
        ws = child_alg.getProperty("OutputWorkspace").value

        self.assertEqual(2.0, ws.readX(0)[0])

    def test_createChildAlgorithm_with_list(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        args = {"FloatInput": [2.3, 4.5], "IntInput": [1, 2, 3], "StringInput": ["test1", "test2"]}
        child_alg = parent_alg.createChildAlgorithm("_ParamTester", **args)
        self.assertIsNotNone(child_alg)

    def test_name_provided_as_arg_and_kwarg_throws(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        args = {"name": "CreateSampleWorkspace"}
        with self.assertRaisesRegex(ValueError, "was specified twice"):
            parent_alg.createChildAlgorithm("_ParamTester", **args)

    def test_name_provided_as_kwarg_twice_throws(self):
        parent_alg = AlgorithmManager.createUnmanaged("Load")
        args = {"name": "CreateSampleWorkspace"}
        with self.assertRaises(TypeError):
            parent_alg.createChildAlgorithm(name="_ParamTester", **args)


if __name__ == "__main__":
    unittest.main()
