# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import AlgorithmManager, MatrixWorkspace
from mantid.kernel import Property, FloatPropertyWithValue, StringPropertyWithValue
from testhelpers import create_algorithm, run_algorithm
import numpy as np


class PropertyWithValueTest(unittest.TestCase):
    # Integration algorithm handle
    _integration = None
    # MaskDetectors algorithm handle
    _mask_dets = None

    def setUp(self):
        if self._integration is None:
            self.__class__._integration = create_algorithm("Integration")
            self.__class__._integration.initialize()
        if self._mask_dets is None:
            self.__class__._mask_dets = create_algorithm("MaskDetectors")
            self.__class__._mask_dets.initialize()

    def test_construction_by_name(self):
        prop = StringPropertyWithValue("testprop", "default")
        self.assertTrue(isinstance(prop, Property))
        self.assertEqual(prop.name, "testprop")
        self.assertEqual(prop.value, "default")

    def test_value_setting_as_string_gives_expected_value_for_correct_type(self):
        prop = self.__class__._integration.getProperty("RangeLower")
        prop.valueAsStr = "15.5"

        self.assertAlmostEqual(15.5, prop.value)

    def test_type_str_is_not_empty(self):
        rangeLower = self.__class__._integration.getProperty("RangeLower")
        self.assertGreater(len(rangeLower.type), 0)

    def test_getproperty_value_returns_derived_type(self):
        data = [1.0, 2.0, 3.0]
        alg = run_algorithm("CreateWorkspace", DataX=data, DataY=data, NSpec=1, UnitX="Wavelength", child=True)
        wksp = alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(wksp, MatrixWorkspace))

    def test_units_string_gives_expected_value_for_ascii_unit(self):
        quantity = FloatPropertyWithValue("Energy", 13.5)
        unit = "mueV"
        quantity.units = unit

        self.assertEqual(unit, quantity.units)

    def test_units_string_gives_expected_value_for_utf8_encoded_bytestring(self):
        quantity = FloatPropertyWithValue("Energy", 13.5)
        unit_bytes = b"\xc2\xb5eV"
        quantity.units = unit_bytes

        self.assertEqual(unit_bytes.decode("utf-8"), quantity.units)

    def test_units_string_gives_expected_unicode_object_from_windows_1252_encoding(self):
        # covers some log units in old files at ISIS
        quantity = FloatPropertyWithValue("Energy", 13.5)
        unit_bytes = b"\xb5eV"
        quantity.units = unit_bytes

        self.assertEqual(unit_bytes.decode("windows-1252"), quantity.units)

    def test_units_raises_error_if_string_cannot_be_decoded(self):
        quantity = FloatPropertyWithValue("Energy", 13.5)
        unit_bytes = b"\x81"  # invalid in both utf-8 and windows 1252
        quantity.units = unit_bytes

        self.assertRaises(RuntimeError, lambda: quantity.units)

    def test_unitsAsBytes_Returns_Same_Bytes_Sequence(self):
        quantity = FloatPropertyWithValue("Energy", 13.5)
        units_bytes = b"\x81"
        quantity.units = units_bytes

        self.assertEqual(units_bytes, quantity.unitsAsBytes)

    def test_set_property_int(self):
        self._integration.setProperty("StartWorkspaceIndex", 5)
        self.assertEqual(self._integration.getProperty("StartWorkspaceIndex").value, 5)

    def test_set_property_float(self):
        self._integration.setProperty("RangeLower", 100.5)
        self.assertAlmostEqual(self._integration.getProperty("RangeLower").value, 100.5)
        self._integration.setProperty("RangeLower", 50)  # Set with an int should still work
        self.assertAlmostEqual(self._integration.getProperty("RangeLower").value, 50)

    def test_set_property_bool(self):
        self._integration.setProperty("IncludePartialBins", True)
        self.assertEqual(self._integration.getProperty("IncludePartialBins").value, True)

    def test_set_property_succeeds_with_python_int_lists(self):
        value = [2, 3, 4, 5, 6]
        self._mask_dets.setProperty("WorkspaceIndexList", value)  # size_t
        idx_list = self._mask_dets.getProperty("WorkspaceIndexList").value
        self.assertEqual(len(idx_list), 5)
        for i in range(5):
            self.assertEqual(idx_list[i], i + 2)
        value.append(7)

        self._mask_dets.setProperty("DetectorList", value)  # integer
        det_list = self._mask_dets.getProperty("DetectorList").value
        self.assertEqual(len(det_list), 6)
        for i in range(6):
            self.assertEqual(det_list[i], i + 2)

    def test_set_array_property_with_single_item_correct_type_suceeds(self):
        self._mask_dets.setProperty("WorkspaceIndexList", 10)

        val = self._mask_dets.getProperty("WorkspaceIndexList").value
        self.assertEqual(10, val)

    def test_set_property_succeeds_with_python_float_lists(self):
        rebin = AlgorithmManager.createUnmanaged("Rebin")
        rebin.initialize()
        input = [0.5, 1.0, 5.5]
        rebin.setProperty("Params", input)
        params = rebin.getProperty("Params").value
        self.assertEqual(len(params), 3)
        for i in range(3):
            self.assertEqual(params[i], input[i])

    def test_set_property_raises_type_error_when_a_list_contains_multiple_types(self):
        values = [2, 3, 4.0, 5, 6]
        self.assertRaises(TypeError, self._mask_dets.setProperty, "WorkspaceIndexList", values)  # size_t

    def test_set_property_of_vector_double_succeeds_with_numpy_array_of_float_type(self):
        self._do_vector_double_numpy_test()

    def test_set_property_of_vector_double_succeeds_with_numpy_array_of_int_type(self):
        self._do_vector_double_numpy_test(True)

    def test_set_property_using_list_extracted_from_other_property_succeeds(self):
        det_list_prop = self._mask_dets.getProperty("DetectorList")
        det_list_prop.valueAsStr = "1,2,3,4,5"

        det_list = det_list_prop.value
        self._mask_dets.setProperty("DetectorList", det_list)

    def test_valueAsPrettyStr(self):
        det_list_prop = self._mask_dets.getProperty("DetectorList")
        one_to_five = "1,2,3,4,5"
        det_list_prop.valueAsStr = one_to_five
        self.assertEqual(det_list_prop.valueAsPrettyStr(0, False), one_to_five)
        self.assertEqual(det_list_prop.valueAsPrettyStr(), "1-5")

        two_ranges = "1,2,3,4,5,6,8,9,10"
        det_list_prop.valueAsStr = two_ranges
        self.assertEqual(det_list_prop.valueAsPrettyStr(0, False), two_ranges)
        self.assertEqual(det_list_prop.valueAsPrettyStr(), "1-6,8-10")

        long_list = ",".join(str(x) for x in range(1, 100))
        det_list_prop.valueAsStr = long_list
        self.assertEqual(det_list_prop.valueAsPrettyStr(0, False), long_list)
        self.assertEqual(det_list_prop.valueAsPrettyStr(0, True), "1-99")
        self.assertEqual(det_list_prop.valueAsPrettyStr(40, True), "1-99")
        result = det_list_prop.valueAsPrettyStr(40, False)
        self.assertEqual(result.startswith("1,2,3,"), True)
        self.assertEqual(result.endswith("98,99"), True)

        # Check the dtype return value
        self.assertEqual(det_list_prop.dtype(), "i")

    def _do_vector_double_numpy_test(self, int_type=False):
        create_ws = AlgorithmManager.createUnmanaged("CreateWorkspace")
        create_ws.initialize()
        if int_type:
            datax = np.arange(10)
        else:
            datax = np.arange(10.0)
        create_ws.setProperty("DataX", datax)
        x_values = create_ws.getProperty("DataX").value
        self.assertEqual(len(x_values), 10)
        for i in range(10):
            self.assertEqual(x_values[i], i)

    def _do_vector_int_numpy_test(self, property_name, dtype=None):
        # Use the maskdetectors alg
        indices = np.arange(6, dtype=dtype)
        self._mask_dets.setProperty(property_name, indices)
        prop_values = self._mask_dets.getProperty(property_name).value
        self.assertEqual(len(prop_values), 6)
        for i in range(6):
            self.assertEqual(prop_values[i], i)

    def test_set_property_of_vector_int_succeeds_with_numpy_array_of_int_type(self):
        # Minor hole with int64 as that technically can't be converted to an int32 without precision loss
        # but I don't think it will be heavily used so we'll see
        self._do_vector_int_numpy_test("DetectorList", np.int32)

    def test_set_property_of_vector_int_succeeds_with_numpy_array_of_int_type(self):
        self._do_vector_int_numpy_test("WorkspaceIndexList")

    def test_property_as_output(self):
        """
        Test that PropertyWithValue can be declared as an output property of an algorithm,
        and that it will be returned as the output when called using the python API function approach
        """
        from mantid.api import AlgorithmFactory, PythonAlgorithm
        from mantid.kernel import Direction, IntPropertyWithValue, ULongLongPropertyWithValue
        from mantid.simpleapi import _create_algorithm_function

        # a large number than cannot fit in a C++ int, requiring u_int64
        BIGINT: int = 125824461545280

        # create an algorithm that returns a ULongLongPropertyWithValue as the output
        # then register it so that it can be called as a function
        class MyAlgorithm(PythonAlgorithm):
            def PyInit(self):
                self.declareProperty(IntPropertyWithValue("InputInt", 0))
                self.declareProperty(ULongLongPropertyWithValue("MyProperty", 0, direction=Direction.Output))

            def PyExec(self):
                self.setProperty("MyProperty", BIGINT)

        AlgorithmFactory.subscribe(MyAlgorithm)
        algo = MyAlgorithm()
        algo.initialize()
        myAlgorithmName = "MyAlgorithm"
        _create_algorithm_function(myAlgorithmName, 1, algo)
        from mantid.simpleapi import MyAlgorithm as MyAlgorithmInMantid

        # call the algorithm, ensure the output is the large integer expected
        ret = MyAlgorithmInMantid(1)
        assert ret == BIGINT
        # clean up the the algorithm manager and verify
        AlgorithmFactory.unsubscribe(myAlgorithmName, 1)
        with self.assertRaises(RuntimeError) as cm:
            MyAlgorithmInMantid(2)
        assert f"not registered {myAlgorithmName}" in str(cm.exception)


if __name__ == "__main__":
    unittest.main()
