import unittest
from mantid.simpleapi import *
from isis_reflectometry.convert_to_wavelength import ConvertToWavelength

class ConvertToWavelengthTest(unittest.TestCase):
    """
    Test the convert to wavelength type.
    """
    def test_construction_from_single_ws(self):
        ws = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        converter = ConvertToWavelength(ws)
        self.assertTrue(converter != None, "Should have been able to make a valid converter from a single workspace")
        DeleteWorkspace(ws)

    def test_construction_from_single_ws_name(self):
        ws = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])

        converter = ConvertToWavelength(ws.getName())
        self.assertTrue(converter != None, "Should have been able to make a valid converter from a single workspace name")
        DeleteWorkspace(ws)

    def test_construction_from_many_workspaces(self):
        ws1 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        ws2 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        converter = ConvertToWavelength([ws1, ws2])
        self.assertTrue(converter != None, "Should have been able to make a valid converter from many workspace objects")
        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)

    def test_construction_from_many_workspace_names(self):
        ws1 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        ws2 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        converter = ConvertToWavelength([ws1.getName(), ws2.getName()])
        self.assertTrue(converter != None, "Should have been able to make a valid converter from many workspace objects")
        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)

    def test_construction_from_comma_separated_workspaces(self):
        ws1 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        ws2 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        comma_separated_names = "%s, %s" % (ws1.getName(), ws2.getName())
        converter = ConvertToWavelength(comma_separated_names)
        self.assertTrue(converter != None, "Should have been able to make a valid converter from many , separated workspace objects")
        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)

    def test_construction_from_semicolon_separated_workspaces(self):
        ws1 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        ws2 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        colon_separated_names = "%s: %s" % (ws1.getName(), ws2.getName())
        converter = ConvertToWavelength(colon_separated_names)
        self.assertTrue(converter != None, "Should have been able to make a valid converter from many : separated workspace objects")
        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)

    def test_sum_workspaces(self):
        ws1 = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        ws2 = CloneWorkspace(ws1)
        ws3 = CloneWorkspace(ws1)
        sum = ConvertToWavelength.sum_workspaces([ws1, ws2, ws3])
        self.assertEqual(set([3,6,9]), set(sum.readY(0)), "Fail to sum workspaces correctly")
        DeleteWorkspace(ws1)
        DeleteWorkspace(ws2)
        DeleteWorkspace(ws3)
        DeleteWorkspace(sum)

    def test_conversion_throws_with_min_wavelength_greater_or_equal_to_max_wavelength(self):
        ws = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        converter = ConvertToWavelength(ws)
        self.assertRaises(ValueError, converter.convert, 1.0, 0.0, (), 0)
        self.assertRaises(ValueError, converter.convert, 1.0, 1.0, (), 0)
        DeleteWorkspace(ws)

    def test_conversion_throws_with_some_flat_background_params_but_not_all(self):
        ws = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        converter = ConvertToWavelength(ws)
        self.assertRaises(ValueError, converter.convert, 0.0, 1.0, (), 0, True)
        DeleteWorkspace(ws)

    def test_conversion_throws_with_min_background_greater_than_or_equal_to_max_background(self):
        ws = CreateWorkspace(DataY=[1,2,3], DataX=[1,2,3])
        converter = ConvertToWavelength(ws)
        self.assertRaises(ValueError, converter.convert, 0.0, 1.0, (), 0, True, 1.0, 1.0)
        DeleteWorkspace(ws)


    def test_crop_range(self):
        original_ws = Load(Filename='INTER00013460')

        # Crop out one spectra
        temp_ws = ConvertToWavelength.crop_range(original_ws, (0, original_ws.getNumberHistograms()-2))
        self.assertEqual(original_ws.getNumberHistograms()-1, temp_ws.getNumberHistograms())

        # Crop out all but 2 spectra from start and end.
        temp_ws = ConvertToWavelength.crop_range(original_ws, ( (0, 1), (3, 4) ) )
        self.assertEqual(2, temp_ws.getNumberHistograms())

        # Crop out all but 2 spectra from start and end. Exactly the same as above, but slightly different tuple syntax
        temp_ws = ConvertToWavelength.crop_range(original_ws, ( ( (0, 1), (3, 4) ) ))
        self.assertEqual(2, temp_ws.getNumberHistograms())

        # Crop out all but 1 spectra
        temp_ws = ConvertToWavelength.crop_range(original_ws, ( (1, 3) ) )
        self.assertEqual(3, temp_ws.getNumberHistograms())
        # First and last dectors are cropped off, so indexes go 2-4 rather than 1-5
        self.assertEqual(2, temp_ws.getDetector(0).getID())
        self.assertEqual(3, temp_ws.getDetector(1).getID())
        self.assertEqual(4, temp_ws.getDetector(2).getID())

        # Test resilience to junk inputs
        self.assertRaises(ValueError, ConvertToWavelength.crop_range, original_ws, 'a')
        self.assertRaises(ValueError, ConvertToWavelength.crop_range, original_ws, (1,2,3))

    @classmethod
    def cropped_x_range(cls, ws, index):
        det_ws_x = ws.readX(index)
        mask = ws.readY(index) != 0 # CropWorkspace will only zero out y values! so we need to translate those to an x range
        cropped_x = det_ws_x[mask]
        return cropped_x[0], cropped_x[-1]

    def test_convert(self):
        ws = Load(Filename='INTER00013460')
        converter = ConvertToWavelength(ws)

        monitor_ws, detector_ws = converter.convert(wavelength_min=0.0, wavelength_max=10.0, detector_workspace_indexes = (2,4), monitor_workspace_index=0, correct_monitor=True, bg_min=2.0, bg_max=8.0)

        self.assertEqual(1, monitor_ws.getNumberHistograms(), "Wrong number of spectra in monitor workspace")
        self.assertEqual(3, detector_ws.getNumberHistograms(), "Wrong number of spectra in detector workspace")
        self.assertEqual("Wavelength", detector_ws.getAxis(0).getUnit().unitID())
        self.assertEqual("Wavelength", monitor_ws.getAxis(0).getUnit().unitID())
        x_min, x_max = ConvertToWavelengthTest.cropped_x_range(detector_ws, 0)

        self.assertTrue(x_min >= 0.0)
        self.assertTrue(x_max <= 10.0)

if __name__ == '__main__':
    unittest.main()