# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import abins
from abins.input import VASPLoader


class AbinsLoadVASPTest(unittest.TestCase, abins.input.Tester):
    def tearDown(self):
        # Remove ref files from .check() calls
        abins.test_helpers.remove_output_files(list_of_names=["_LoadVASP"])

    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            bad_vasp_reader = VASPLoader(input_ab_initio_filename="NonExistingFile.txt")
            bad_vasp_reader.read_vibrational_or_phonon_data()

        with self.assertRaises(TypeError):
            _ = VASPLoader(input_ab_initio_filename=1)

    # Not a real vibration calc; check the appropriate error is raised
    def test_singlepoint_input(self):
        filename = abins.test_helpers.find_file("ethane_singlepoint.xml")
        bad_vasp_reader = VASPLoader(input_ab_initio_filename=filename)
        with self.assertRaisesRegexp(ValueError, "Could not find a 'calculation' block containing a 'dynmat' block in VASP XML file\\."):
            bad_vasp_reader.read_vibrational_or_phonon_data()

    # IBRION=8 from optimised structure
    def test_xml_dfpt(self):
        self.check(name="ethane_LoadVASP", loader=VASPLoader)

    def test_outcar_dfpt(self):
        self.check(name="ethane_LoadVASP_outcar", loader=VASPLoader, extension="OUTCAR")

    # IBRION=6 including optimisation steps
    def test_xml_finitedisplacement(self):
        self.check(name="ozone_LoadVASP", loader=VASPLoader)

    # IBRION=5 including frozen atoms
    def test_xml_frozenatoms(self):
        self.check(name="methane_surface_LoadVASP", loader=VASPLoader)

    # IBRION=6 including frozen atoms
    def test_xml_frozenatoms_ibrion6(self):
        self.check(name="si_ibrion6_selective_LoadVASP", loader=VASPLoader)

    # IBRION=8 from VASP 6 (in THz units)
    def test_xml_vasp6(self):
        self.check(name="ethanol_VASP6_LoadVASP", loader=VASPLoader)
