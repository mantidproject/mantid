from __future__ import (absolute_import, division, print_function)

import mantid
import unittest

from isis_powder.routines import sample_details

from six_shim import assertRaisesRegex

class ISISPowderSampleDetailsTest(unittest.TestCase):
    def test_constructor(self):
        expected_height = 1.1
        expected_radius = 2.2
        expected_center = [3.3, 4.4, 5.5]

        # Check easiest case
        sample_details_obj = sample_details.SampleDetails(height=expected_height, radius=expected_radius,
                                                          center=expected_center)
        self.assertEqual(sample_details_obj.height, expected_height)
        self.assertEqual(sample_details_obj.radius, expected_radius)
        self.assertEqual(sample_details_obj.center, expected_center)

        # Does it handle ints correctly
        height_radius_int = 1
        center_int = [2, 3, 4]
        sample_details_obj_int = sample_details.SampleDetails(height=height_radius_int, radius=height_radius_int,
                                                              center=center_int)
        self.assertTrue(isinstance(sample_details_obj.height, float))
        self.assertTrue(isinstance(sample_details_obj.radius, float))
        self.assertEqual(sample_details_obj_int.height, float(height_radius_int))
        self.assertEqual(sample_details_obj_int.radius, float(height_radius_int))
        self.assertEqual(sample_details_obj_int.center, [2.0, 3.0, 4.0])

        # Does it handle strings correctly
        height_radius_string = "5"
        center_string = ["2.0", "3.0", "5.0"]
        sample_details_obj_str = sample_details.SampleDetails(height=height_radius_string, radius=height_radius_string,
                                                              center=center_string)
        self.assertTrue(isinstance(sample_details_obj.height, float))
        self.assertTrue(isinstance(sample_details_obj.radius, float))
        self.assertEqual(sample_details_obj_str.height, float(height_radius_string))
        self.assertEqual(sample_details_obj_str.radius, float(height_radius_string))
        self.assertEqual(sample_details_obj_str.center, [2.0, 3.0, 5.0])

    def test_constructor_non_numeric_input(self):
        good_input = 1.0
        good_center_input = [1.0, 2.0, 3.0]
        empty_input_value = ''
        char_input_value = 'a'

        # Check it handles empty input
        with assertRaisesRegex(self, ValueError, "Could not convert the height to a number"):
            sample_details.SampleDetails(height=empty_input_value, radius=good_input, center=good_center_input)

        # Does it handle bad input and tell us what we put in
        with assertRaisesRegex(self, ValueError, ".*to a number. The input was: '" + char_input_value + "'"):
            sample_details.SampleDetails(height=char_input_value, radius=good_input, center=good_center_input)

        # Does it indicate which field was incorrect
        with assertRaisesRegex(self, ValueError, "radius"):
            sample_details.SampleDetails(height=good_input, radius=char_input_value, center=good_center_input)

        # Can it handle bad center values
        with assertRaisesRegex(self, ValueError, "center"):
            sample_details.SampleDetails(height=good_input, radius=good_input, center=["", 2, 3])

        # Does it throw if were not using a list for the input
        with assertRaisesRegex(self, ValueError, "must be specified as a list of X, Y, Z"):
            sample_details.SampleDetails(height=good_input, radius=good_input, center=1)

        # Does it throw if we are using a list of incorrect length (e.g. not 3D)
        with assertRaisesRegex(self, ValueError, "must have three values corresponding to"):
            sample_details.SampleDetails(height=good_input, radius=good_input, center=[])
        with assertRaisesRegex(self, ValueError, "must have three values corresponding to"):
            sample_details.SampleDetails(height=good_input, radius=good_input, center=[1, 2])
        with assertRaisesRegex(self, ValueError, "must have three values corresponding to"):
            sample_details.SampleDetails(height=good_input, radius=good_input, center=[1, 2, 3, 4])

    def test_constructor_with_impossible_val(self):
        good_input = 1
        good_center_input = [1, 2, 3]
        zero_value = 0
        negative_value = -0.0000001
        negative_int = -1
        negative_string = "-1"

        # Check it handles zero
        with assertRaisesRegex(self, ValueError, "The value set for height was: 0"):
            sample_details.SampleDetails(height=zero_value, radius=good_input, center=good_center_input)

        # Very small negative
        with assertRaisesRegex(self, ValueError, "which is impossible for a physical object"):
            sample_details.SampleDetails(height=good_input, radius=negative_value, center=good_center_input)

        # Integer negative
        with assertRaisesRegex(self, ValueError, "The value set for height was: -1"):
            sample_details.SampleDetails(height=negative_int, radius=good_input, center=good_center_input)

        # String negative
        with assertRaisesRegex(self, ValueError, "The value set for radius was: -1"):
            sample_details.SampleDetails(height=good_input, radius=negative_string, center=good_center_input)

        # Test center lists correctly detect incorrect values
        with assertRaisesRegex(self, ValueError, "The value set for center was: 0"):
            sample_details.SampleDetails(height=good_input, radius=good_input,
                                         center=[zero_value, good_input, good_input])

    def test_set_material(self):
        sample_details_obj = sample_details.SampleDetails(height=1.0, radius=1.0, center=[2, 3, 4])

        # Check that we can only set a material once. We will test the underlying class elsewhere
        sample_details_obj.set_material(chemical_formula='V')
        self.assertIsNotNone(sample_details_obj._material_object)

        # Check that the material is now immutable
        with assertRaisesRegex(self, RuntimeError, "The material has already been set to the above details"):
            sample_details_obj.set_material(chemical_formula='V')

        # Check resetting it works
        sample_details_obj.reset_sample_material()
        self.assertIsNone(sample_details_obj._material_object)

        # And ensure setting it for a second time works
        sample_details_obj.set_material(chemical_formula='V')
        self.assertIsNotNone(sample_details_obj._material_object)

    def test_set_material_properties(self):
        sample_details_obj = sample_details.SampleDetails(height=1.0, radius=1.0, center=[2, 3, 5])

        self.assertIsNone(sample_details_obj._material_object)

        # Check we cannot set a material property without setting the underlying material
        with assertRaisesRegex(self, RuntimeError, "The material has not been set"):
            sample_details_obj.set_material_properties(absorption_cross_section=1.0, scattering_cross_section=2.0)

        # Check that with a material object we are allowed to set material properties
        sample_details_obj.set_material(chemical_formula='V')
        # We will test the immutability of the underlying object elsewhere
        sample_details_obj.set_material_properties(scattering_cross_section=2.0, absorption_cross_section=3.0)

    def test_material_constructor(self):
        chemical_formula_one_char_element = 'V'
        chemical_formula_two_char_element = 'Si'
        chemical_formula_complex = 'V Si'  # Yes, this isn't a sensible input but for our tests it will do
        numeric_density_sample = 1.234

        material_obj_one_char = sample_details._Material(chemical_formula=chemical_formula_one_char_element)
        self.assertIsNotNone(material_obj_one_char)
        self.assertEqual(material_obj_one_char._chemical_formula, chemical_formula_one_char_element)
        self.assertIsNone(material_obj_one_char._numeric_density)

        # Also check that the absorption and scattering X sections have not been set
        self.assertIsNone(material_obj_one_char._absorption_cross_section)
        self.assertIsNone(material_obj_one_char._scattering_cross_section)
        self.assertFalse(material_obj_one_char._is_material_props_set)

        # Check if it accepts two character elements without numeric density
        material_obj_two_char = sample_details._Material(chemical_formula=chemical_formula_two_char_element)
        self.assertIsNotNone(material_obj_two_char)
        self.assertEqual(material_obj_two_char._chemical_formula, chemical_formula_two_char_element)
        self.assertIsNone(material_obj_two_char._numeric_density)

        # Check it stores numeric density if passed
        material_obj_numeric_density = sample_details._Material(chemical_formula=chemical_formula_two_char_element,
                                                                numeric_density=numeric_density_sample)
        self.assertEqual(material_obj_numeric_density._numeric_density, numeric_density_sample)

        # Check that it raises an error if we have a non-elemental formula without numeric density
        with assertRaisesRegex(self, ValueError, "A numeric density formula must be set on a chemical formula"):
            sample_details._Material(chemical_formula=chemical_formula_complex)

        # Check it constructs if it is given the numeric density too
        material_obj_num_complex_formula = sample_details._Material(chemical_formula=chemical_formula_complex,
                                                                    numeric_density=numeric_density_sample)
        self.assertEqual(material_obj_num_complex_formula._chemical_formula, chemical_formula_complex)
        self.assertEqual(material_obj_num_complex_formula._numeric_density, numeric_density_sample)

    def test_material_set_properties(self):
        bad_absorb = '-1'
        bad_scattering = 0

        good_absorb = '1'
        good_scattering = 2.0

        material_obj = sample_details._Material(chemical_formula='V')
        with assertRaisesRegex(self, ValueError, "absorption_cross_section was: -1 which is impossible for a physical "
                                                 "object"):
            material_obj.set_material_properties(abs_cross_sect=bad_absorb, scattering_cross_sect=good_scattering)

        # Check the immutability flag has not been set on a failure
        self.assertFalse(material_obj._is_material_props_set)

        with assertRaisesRegex(self, ValueError, "scattering_cross_section was: 0"):
            material_obj.set_material_properties(abs_cross_sect=good_absorb, scattering_cross_sect=bad_scattering)

        # Check nothing has been set yet
        self.assertIsNone(material_obj._absorption_cross_section)
        self.assertIsNone(material_obj._scattering_cross_section)

        # Set the object this time
        material_obj.set_material_properties(abs_cross_sect=good_absorb, scattering_cross_sect=good_scattering)
        self.assertTrue(material_obj._is_material_props_set)
        self.assertEqual(material_obj._absorption_cross_section, float(good_absorb))
        self.assertEqual(material_obj._scattering_cross_section, float(good_scattering))

        # Check we cannot set it twice and fields do not change
        with assertRaisesRegex(self, RuntimeError, "The material properties have already been set"):
            material_obj.set_material_properties(abs_cross_sect=999, scattering_cross_sect=999)
        self.assertEqual(material_obj._absorption_cross_section, float(good_absorb))
        self.assertEqual(material_obj._scattering_cross_section, float(good_scattering))

if __name__ == "__main__":
    unittest.main()
