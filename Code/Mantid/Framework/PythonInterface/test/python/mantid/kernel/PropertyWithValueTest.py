import unittest
from mantid.api import AlgorithmManager, MatrixWorkspace
from testhelpers import run_algorithm
import numpy as np

class PropertyWithValueTest(unittest.TestCase):

    # Integration algorithm handle
    _integration = None
    # MaskDetectors algorithm handle
    _mask_dets = None

    def setUp(self):
        if self._integration is None:
            self.__class__._integration = AlgorithmManager.createUnmanaged("Integration")
            self.__class__._integration.initialize()
        if self._mask_dets is None:
            self.__class__._mask_dets = AlgorithmManager.createUnmanaged("MaskDetectors")
            self.__class__._mask_dets.initialize()

    def test_value_setting_as_string_gives_expected_value_for_correct_type(self):
        prop = self.__class__._integration.getProperty("RangeLower")
        prop.valueAsStr = "15.5"

        self.assertAlmostEqual(15.5, prop.value)

    def test_type_str_is_not_empty(self):
        rangeLower=self.__class__._integration.getProperty("RangeLower")
        self.assertTrue(len(rangeLower.type) > 0)

    def test_getproperty_value_returns_derived_type(self):
        data = [1.0,2.0,3.0]
        alg = run_algorithm('CreateWorkspace',DataX=data,DataY=data,NSpec=1,UnitX='Wavelength',child=True)
        wksp = alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(wksp,MatrixWorkspace))

    def test_set_property_int(self):
        self._integration.setProperty("StartWorkspaceIndex", 5)
        self.assertEquals(self._integration.getProperty("StartWorkspaceIndex").value, 5)

    def test_set_property_float(self):
        self._integration.setProperty("RangeLower", 100.5)
        self.assertAlmostEqual(self._integration.getProperty("RangeLower").value, 100.5)
        self._integration.setProperty("RangeLower", 50) # Set with an int should still work
        self.assertAlmostEqual(self._integration.getProperty("RangeLower").value, 50)

    def test_set_property_bool(self):
        self._integration.setProperty("IncludePartialBins", True)
        self.assertEquals(self._integration.getProperty("IncludePartialBins").value, True)

    def test_set_property_succeeds_with_python_int_lists(self):
        value = [2,3,4,5,6]
        self._mask_dets.setProperty("WorkspaceIndexList", value) #size_t
        idx_list = self._mask_dets.getProperty("WorkspaceIndexList").value
        self.assertEquals(len(idx_list), 5)
        for i in range(5):
            self.assertEquals(idx_list[i], i+2)
        value.append(7)

        self._mask_dets.setProperty("DetectorList", value) #integer
        det_list = self._mask_dets.getProperty("DetectorList").value
        self.assertEquals(len(det_list), 6)
        for i in range(6):
            self.assertEquals(det_list[i], i+2)

    def test_set_array_property_with_single_item_correct_type_suceeds(self):
        self._mask_dets.setProperty("WorkspaceIndexList", 10)

        val =  self._mask_dets.getProperty("WorkspaceIndexList").value
        self.assertEquals(10, val)

    def test_set_property_succeeds_with_python_float_lists(self):
        rebin = AlgorithmManager.createUnmanaged("Rebin")
        rebin.initialize()
        input = [0.5,1.0,5.5]
        rebin.setProperty('Params',input)
        params = rebin.getProperty('Params').value
        self.assertEquals(len(params), 3)
        for i in range(3):
            self.assertEquals(params[i], input[i])

    def test_set_property_raises_type_error_when_a_list_contains_multiple_types(self):
        values = [2,3,4.0,5,6]
        self.assertRaises(TypeError, self._mask_dets.setProperty, "WorkspaceIndexList", values) #size_t

    def test_set_property_of_vector_double_succeeds_with_numpy_array_of_float_type(self):
        self._do_vector_double_numpy_test()

    def test_set_property_of_vector_double_succeeds_with_numpy_array_of_int_type(self):
        self._do_vector_double_numpy_test(True)

    def test_set_property_using_list_extracted_from_other_property_succeeds(self):
        det_list_prop = self._mask_dets.getProperty("DetectorList")
        det_list_prop.valueAsStr = "1,2,3,4,5"

        det_list = det_list_prop.value
        self._mask_dets.setProperty("DetectorList", det_list)

    def _do_vector_double_numpy_test(self, int_type=False):
        create_ws = AlgorithmManager.createUnmanaged('CreateWorkspace')
        create_ws.initialize()
        if int_type:
            datax = np.arange(10)
        else:
            datax = np.arange(10.0)
        create_ws.setProperty('DataX', datax)
        x_values = create_ws.getProperty('DataX').value
        self.assertEquals(len(x_values), 10)
        for i in range(10):
            self.assertEquals(x_values[i], i)

    def _do_vector_int_numpy_test(self, property_name, dtype=None):
        # Use the maskdetectors alg
        indices = np.arange(6,dtype=dtype)
        self._mask_dets.setProperty(property_name, indices)
        prop_values = self._mask_dets.getProperty(property_name).value
        self.assertEquals(len(prop_values), 6)
        for i in range(6):
            self.assertEquals(prop_values[i], i)

    def test_set_property_of_vector_int_succeeds_with_numpy_array_of_int_type(self):
        # Minor hole with int64 as that technically can't be converted to an int32 without precision loss
        # but I don't think it will be heavily used so we'll see
        self._do_vector_int_numpy_test('DetectorList', np.int32)

    def test_set_property_of_vector_int_succeeds_with_numpy_array_of_int_type(self):
        self._do_vector_int_numpy_test('WorkspaceIndexList')

if __name__ == '__main__':
    unittest.main()
