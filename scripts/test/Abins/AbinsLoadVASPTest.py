# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import mantid  # has to be imported so that AbinsModules can be found
import AbinsModules

class AbinsLoadVASPTest(unittest.TestCase, AbinsModules.GeneralLoadAbInitioTester):
    def tearDown(self):
        # Remove ref files from .check() calls
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["LoadVASP"])

    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            bad_vasp_reader = AbinsModules.LoadVASP(input_ab_initio_filename="NonExistingFile.txt")
            bad_vasp_reader.read_vibrational_or_phonon_data()

        with self.assertRaises(ValueError):
            _ = AbinsModules.LoadVASP(input_ab_initio_filename=1)


    # Not a real vibration calc; check the appropriate error is raised
    def test_singlepoint_input(self):
        filename = AbinsModules.AbinsTestHelpers.find_file("ethane_singlepoint.xml")
        bad_vasp_reader = AbinsModules.LoadVASP(input_ab_initio_filename=filename)
        with self.assertRaisesRegexp(ValueError,
                                     "Could not find a 'calculation' block containing a "
                                     "'dynmat' block in VASP XML file\."):
            bad_vasp_reader.read_vibrational_or_phonon_data()

    # IBRION=8 from optimised structure
    def test_xml_dfpt(self):
        self.check(name='ethane_LoadVASP', loader=AbinsModules.LoadVASP)

    def test_outcar_dfpt(self):
        self.check(name='ethane_LoadVASP_outcar', loader=AbinsModules.LoadVASP,
                   extension='OUTCAR')

    # IBRION=6 including optimisation steps
    def test_xml_finitedisplacement(self):
        self.check(name='ozone_LoadVASP', loader=AbinsModules.LoadVASP)

if __name__ == '__main__':
    unittest.main()
