# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_obs_model import \
    DNSObsModel
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_tof_powder_dataset \
    import DNSTofDataset
from mantidqtinterfaces.dns_powder_tof.script_generator. \
    common_script_generator_model import DNSScriptGeneratorModel
from mantidqtinterfaces.dns_powder_tof.script_generator. \
    tof_powder_script_generator_model import DNSTofPowderScriptGeneratorModel
from mantidqtinterfaces.dns_powder_tof.helpers.helpers_for_testing import (
    get_fake_tof_options, get_file_selector_full_data, get_paths)


class DNSTofPowderScriptGeneratorModelTest(unittest.TestCase):
    # pylint: disable=protected-access, too-many-public-methods
    parent = None
    model = None

    @classmethod
    def setUpClass(cls):
        cls.parent = mock.Mock()
        cls.model = DNSTofPowderScriptGeneratorModel(cls.parent)
        cls.model._tof_opt = get_fake_tof_options()
        cls.model._sample_data = mock.Mock()
        cls.model._standard_data = mock.Mock()
        cls.model._sample_data.format_dataset = mock.Mock(return_value='test2')
        cls.model._standard_data.format_dataset = mock.Mock(
            return_value='test')
        cls.model._standard_data.get_empty_scan_name.return_value = \
            'test_empty.d_dat'
        cls.model._standard_data.get_vana_scan_name.return_value = \
            'test_vana.d_dat'
        cls.model._sample_data.get_sample_filename.return_value = \
            'test_sample.d_dat'
        cls.model._standard_data.get_nb_vana_banks.return_value = 1
        cls.model._standard_data.get_nb_empty_banks.return_value = 1
        cls.model._sample_data.get_nb_banks.return_value = 5

    def test___init__(self):
        self.assertIsInstance(self.model, DNSTofPowderScriptGeneratorModel)
        self.assertIsInstance(self.model, DNSScriptGeneratorModel)
        self.assertIsInstance(self.model, DNSObsModel)
        self.assertEqual(self.model._nb_empty_banks, 0)
        self.assertEqual(self.model._nb_vana_banks, 0)
        self.assertEqual(self.model._nb_banks, 0)
        self.assertIsNone(self.model._script)
        self.assertIsInstance(self.model._tof_opt, dict)
        self.assertEqual(self.model._tof_opt['q_step'], 0.025)

    def test_validate_tof_options(self):
        self.assertTrue(self.model._validate_tof_options())
        self.model._tof_opt['dE_step'] = 0
        self.assertFalse(self.model._validate_tof_options())
        self.model._tof_opt['dE_step'] = 1
        self.model._tof_opt['q_step'] = 0
        self.assertFalse(self.model._validate_tof_options())
        self.model._tof_opt['q_step'] = 1
        self.model._tof_opt['q_max'] = -100
        self.assertFalse(self.model._validate_tof_options())
        self.model._tof_opt['q_max'] = 100
        self.model._tof_opt['dE_max'] = -100
        self.assertFalse(self.model._validate_tof_options())
        self.model._tof_opt['dE_max'] = 100
        self.assertTrue(self.model._validate_tof_options())

    def test_validate_number_empty_banks(self):
        self.model._tof_opt['corrections'] = 1
        self.model._standard_data._nb_vana_banks = 1
        self.model._standard_data._nb_empty_banks = 1
        self.model._sample_data._nb_banks = 5
        self.model._nb_vana_banks = 1
        self.model._nb_empty_banks = 1
        self.assertTrue(self.model._validate_nb_empty_banks())
        self.model._nb_empty_banks = 0
        self.assertFalse(self.model._validate_nb_empty_banks())
        self.model._tof_opt['corrections'] = 0
        self.assertTrue(self.model._validate_nb_empty_banks())
        self.model._tof_opt['corrections'] = 1
        self.model._tof_opt['subtract_vana_back'] = 0
        self.assertFalse(self.model._validate_nb_empty_banks())
        self.model._tof_opt['subtract_sample_back'] = 0
        self.assertTrue(self.model._validate_nb_empty_banks())
        self.model._tof_opt['subtract_vana_back'] = 1
        self.assertFalse(self.model._validate_nb_empty_banks())
        self.model._tof_opt['subtract_vana_back'] = 0
        self.model._tof_opt['corrections'] = 0
        self.assertTrue(self.model._validate_nb_empty_banks())
        self.model._tof_opt = get_fake_tof_options()

    def test_validate_number_vana_banks(self):
        self.model._tof_opt['corrections'] = 1
        self.model._standard_data._nb_vana_banks = 1
        self.model._standard_data._nb_empty_banks = 1
        self.model._sample_data._nb_banks = 5
        self.model._nb_vana_banks = 1
        self.assertTrue(self.model._validate_nb_vana_banks())
        self.model._nb_vana_banks = 0
        self.assertFalse(self.model._validate_nb_vana_banks())
        self.model._tof_opt['corrections'] = 0
        self.assertTrue(self.model._validate_nb_vana_banks())
        self.model._tof_opt['corrections'] = 1
        self.model._tof_opt['det_efficiency'] = 0
        self.assertTrue(self.model._validate_nb_vana_banks())
        self.model._tof_opt = get_fake_tof_options()

    def test_check_vana_cor(self):
        self.model._nb_vana_banks = 0
        self.assertFalse(self.model._check_vana_cor())
        self.model._nb_vana_banks = 1
        self.assertTrue(self.model._check_vana_cor())
        self.model._tof_opt['det_efficiency'] = 0
        self.assertFalse(self.model._check_vana_cor())
        self.model._tof_opt['det_efficiency'] = 1
        self.model._tof_opt['corrections'] = 0
        self.assertFalse(self.model._check_vana_cor())
        self.model._tof_opt = get_fake_tof_options()

    def test_error_in_input(self):
        self.model._nb_vana_banks = 1
        self.model._nb_empty_banks = 1
        self.assertEqual(self.model._error_in_input(), '')
        self.model._tof_opt['dE_step'] = 0
        self.assertEqual(self.model._error_in_input()[0:3], 'Bin')
        self.model._tof_opt['dE_step'] = 1
        self.model._nb_vana_banks = 0
        self.assertEqual(self.model._error_in_input()[0:4], 'Vana')
        self.model._nb_vana_banks = 1
        self.model._nb_empty_banks = 0
        self.assertEqual(self.model._error_in_input()[0:4], 'Back')
        self.model._tof_opt = get_fake_tof_options()

    def test_get_vana_string(self):
        self.model._vana_cor = False
        self.assertEqual(self.model._get_vana_string(), '')
        self.model._vana_cor = True
        self.assertEqual(self.model._get_vana_string()[10:20], "'vana_temp")

    def test_get_background_string(self):
        self.model._bg_cor = False
        self.assertEqual(self.model._get_back_string(), '')
        self.model._bg_cor = True
        self.assertEqual(self.model._get_back_string()[10:20], "'ecVanaFac")

    def test_get_background_tof_string(self):
        self.model._bg_cor = False
        self.assertEqual(self.model._get_back_tof_string(), '')
        self.model._bg_cor = True
        self.assertEqual(self.model._get_back_tof_string()[10:20], "'ecSampleF")

    def test_get_parameter_lines(self):
        self.model._bg_cor = True
        self.model._vana_cor = True
        test_v = self.model._get_parameter_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 2)

        test_string = ("params = {'e_channel': 0,"
                       "\n          'wavelength': 4.74,"
                       "\n          'delete_raw': True,"
                       "\n          'vana_temperature': 295,"
                       "\n          'ecVanaFactor': 1,"
                       "\n          'ecSampleFactor': 1,}")
        self.assertEqual(test_v[0], test_string)

    def test_get_binning_lines(self):
        self.model._tof_opt = get_fake_tof_options()
        test_v = self.model._get_binning_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 2)
        test_string = ("bins = {'q_min': 0.185,\n"
                       "        'q_max': 2.837,\n"
                       "        'q_step': 0.025,\n"
                       "        'dE_min': -3.141,\n"
                       "        'dE_max': 3.141,\n"
                       "        'dE_step': 0.073}")
        self.assertEqual(test_v[0], test_string)

    def test_get_header_lines(self):
        test_v = self.model._get_header_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 5)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(test_v[0][0:6], 'from m')

    def test_get_sample_data_lines(self):
        test_v = self.model._get_sample_data_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 1)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(test_v[0], 'sample_data = test2')

    def test_get_standard_data_lines(self):
        self.model._tof_opt['corrections'] = 0
        self.assertEqual(self.model._get_standard_data_lines(), [''])
        self.model._tof_opt['corrections'] = 1
        test_v = self.model._get_standard_data_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 1)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(test_v[0], 'standard_data = test')

    def test_get_load_data_lines(self):
        self.model._bg_cor = False
        self.model._vana_cor = False
        test_v = self.model._get_load_data_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 2)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(test_v[0][20:35], 'a["test_sample.')
        self.model._bg_cor = True
        self.model._vana_cor = True
        test_v = self.model._get_load_data_lines()
        print(test_v)
        self.assertEqual(len(test_v), 4)
        self.assertEqual(test_v[1][20:35], 'ata["test_empty')
        self.assertEqual(test_v[2][20:35], 'ata["test_vana.')

    def test_get_normalisation_lines(self):
        self.model._tof_opt['norm_monitor'] = 0
        test_v = self.model._get_normalisation_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 1)
        self.assertEqual(test_v, ['data1 = mtd["raw_data1"]'])
        self.model._tof_opt['norm_monitor'] = 1
        test_v = self.model._get_normalisation_lines()
        self.assertEqual(len(test_v), 2)
        self.assertEqual(test_v[0], '# normalize')

    def test_get_subtract_empty_lines(self):
        self.model._bg_cor = 0
        test_v = self.model._get_subtract_empty_lines()
        self.assertIsInstance(test_v, list)
        self.assertFalse(test_v)
        self.model._tof_opt['subtract_sample_back'] = False
        self.model._bg_cor = 1
        self.model._nb_empty_banks = 1
        self.model._nb_banks = 1
        test_v = self.model._get_subtract_empty_lines()
        self.assertEqual(len(test_v), 2)
        self.model._nb_banks = 5
        test_v = self.model._get_subtract_empty_lines()
        self.assertEqual(len(test_v), 4)
        self.assertEqual(test_v[3], 'ec = ec[0]')
        self.model._tof_opt['subtract_sample_back'] = True
        test_v = self.model._get_subtract_empty_lines()
        self.assertEqual(len(test_v), 7)
        self.assertEqual(test_v[4], '# subtract empty can')
        self.model._tof_opt = get_fake_tof_options()

    def test_get_vana_ec_subtract_lines(self):
        self.model._tof_opt['subtract_vana_back'] = 0
        self.model._bg_cor = 1
        test_v = self.model._get_vana_ec_subst_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(test_v, [''])
        self.model._tof_opt['subtract_vana_back'] = 1
        self.model._bg_cor = 0
        self.assertEqual(self.model._get_vana_ec_subst_lines(), [''])
        self.model._bg_cor = 1
        test_v = self.model._get_vana_ec_subst_lines()
        self.assertEqual(test_v, ["vanadium = vanadium - ec"])
        self.model._tof_opt['vana_back_factor'] = 0
        test_v = self.model._get_vana_ec_subst_lines()
        testlist = ["vanadium = vanadium - ec * params['ecVanaFactor']"]
        self.assertEqual(test_v, testlist)
        self.model._tof_opt['vana_back_factor'] = 1

    def test_get_only_one_vana_lines(self):
        self.model._nb_vana_banks = 1
        self.model._nb_banks = 1
        test_v = self.model._get_only_one_vana_lines()
        self.assertIsInstance(test_v, list)
        self.assertFalse(test_v)
        self.model._nb_vana_banks = 0
        test_v = self.model._get_only_one_vana_lines()
        self.assertEqual(len(test_v), 3)
        self.assertEqual(test_v[0], '# only one vanadium bank position')

    def test_get_epp_and_coef_lines(self):
        test_v = self.model._get_epp_and_coef_lines()
        self.assertIsInstance(test_v, list)
        self.assertIsInstance(test_v[2], str)
        self.assertEqual(len(test_v), 3)
        self.assertEqual(test_v[0][0:10], "# detector")

    def test_get_corr_epp_lines(self):
        self.model._tof_opt['correct_elastic_peak_position'] = 0
        test_v = self.model._get_corr_epp_lines()
        self.assertIsInstance(test_v, list)
        self.assertFalse(test_v)
        self.model._tof_opt['correct_elastic_peak_position'] = 1
        test_v = self.model._get_corr_epp_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 4)
        self.assertIsInstance(test_v[2], str)
        self.assertEqual(test_v[1][0:10], "# correct ")

    def test_get_bad_det_lines(self):
        self.model._nb_vana_banks = 1
        self.model._nb_banks = 5
        test_v = self.model._get_bad_det_lines()
        self.assertIsInstance(test_v, list)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(len(test_v), 1)
        self.assertEqual(test_v[0][33:41], 'coefs.ex')
        self.model._nb_vana_banks = 5
        test_v = self.model._get_bad_det_lines()
        self.assertIsInstance(test_v, list)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(len(test_v), 1)
        self.assertEqual(test_v[0][33:41], 'coefs[0]')
        self.model._nb_vana_banks = 1
        self.model._nb_banks = 1
        test_v = self.model._get_bad_det_lines()
        self.assertEqual(test_v[0][33:41], 'coefs[0]')
        self.model._nb_banks = 5

    def test_get_mask_det_lines(self):
        self.model._nb_vana_banks = 1
        self.model._nb_banks = 5
        self.model._tof_opt['mask_bad_detectors'] = 0
        self.assertEqual(self.model._get_mask_det_lines(), [''])
        self.model._tof_opt['mask_bad_detectors'] = 1
        test_v = self.model._get_mask_det_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 5)
        self.assertEqual(test_v[0], '# get list of bad detectors')

    def test_get_det_eff_cor_lines(self):
        test_v = self.model._get_det_eff_cor_lines()
        self.assertIsInstance(test_v, list)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(len(test_v), 2)
        self.assertEqual(test_v[1], 'data1 = Divide(data1, coefs)')

    def test_get_vana_lines(self):
        self.model._standard_data._nb_vana_banks = 1
        self.model._standard_data._nb_empty_banks = 1
        self.model._sample_data._nb_banks = 5
        self.model._tof_opt = get_fake_tof_options()
        self.model._vana_cor = 0
        self.assertEqual(self.model._get_vana_lines(), [''])
        self.model._vana_cor = 1
        test_v = self.model._get_vana_lines()
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 19)
        test_string = 'vanadium =  MonitorEfficiencyCorUser("raw_vanadium")'
        self.assertEqual(test_v[0], test_string)

    def test_get_energy_print_lines(self):
        test_v = self.model._get_energy_print_lines()
        self.assertIsInstance(test_v, list)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(len(test_v), 4)
        test_string = "Ei = data1[0].getRun().getLogData('Ei').value"
        self.assertEqual(test_v[1], test_string)

    def test_get_sqw_lines(self):
        test_v = self.model._get_sqw_lines()
        self.assertIsInstance(test_v, list)
        self.assertIsInstance(test_v[0], str)
        self.assertEqual(len(test_v), 5)
        test_string = "convert_to_de('data1', Ei)"
        self.assertEqual(test_v[1], test_string)

    def test_get_save_lines(self):
        paths = get_paths()
        test_v = self.model._get_save_lines(paths)
        self.assertEqual(test_v, [])
        paths['export'] = True
        test_v = self.model._get_save_lines(paths)
        self.assertIsInstance(test_v, list)
        self.assertEqual(len(test_v), 2)
        self.assertEqual(test_v[0][25:28], '123')
        self.assertEqual(test_v[0][0:9], 'SaveAscii')
        self.assertEqual(test_v[1][25:28], '123')
        self.assertEqual(test_v[1][0:9], 'SaveNexus')

    def test_check_if_to_save(self):
        paths = get_paths()
        self.assertEqual(self.model._check_if_to_save(paths), [0, 0])
        paths['export'] = True
        self.assertEqual(self.model._check_if_to_save(paths), [1, 1])
        paths['ascii'] = False
        paths['nexus'] = False
        self.assertEqual(self.model._check_if_to_save(paths), [0, 0])
        paths['nexus'] = True
        self.assertEqual(self.model._check_if_to_save(paths), [0, 1])
        paths['ascii'] = True
        paths['nexus'] = False
        self.assertEqual(self.model._check_if_to_save(paths), [1, 0])
        paths['nexus'] = False
        paths['export_dir'] = ''
        self.assertEqual(self.model._check_if_to_save(paths), [0, 0])

    def test_setup_sample_data(self):
        file_selector = get_file_selector_full_data()
        paths = get_paths()
        self.model._sample_data = None
        self.model._nb_banks = None
        self.model._setup_sample_data(paths, file_selector)
        self.assertIsInstance(self.model._sample_data, DNSTofDataset)
        self.assertEqual(self.model._nb_banks, 1)
        self.model._sample_data = mock.Mock()
        self.model._sample_data.format_dataset = mock.Mock(
            return_value='test2')
        self.model._sample_data.sample_filename = 'test_sample.d_dat'
        self.model._sample_data.nb_banks = 5

    def test_setup_standard_data(self):
        file_selector = get_file_selector_full_data()
        paths = get_paths()
        self.model._standard_data.empty_filename = None
        self.model._standard_data.vana_filename = None
        self.model._sample_data.sample_filename = None
        self.model._standard_data._nb_vana_banks = None
        self.model._standard_data._nb_empty_banks = None
        self.model._tof_opt['corrections'] = 0
        self.model._setup_standard_data(paths, file_selector)
        self.assertEqual(self.model._nb_vana_banks, 0)
        self.assertEqual(self.model._nb_empty_banks, 0)
        self.model._tof_opt['corrections'] = 1
        self.model._setup_standard_data(paths, file_selector)
        self.assertEqual(self.model._nb_vana_banks, 2)
        self.assertEqual(self.model._nb_empty_banks, 1)
        self.assertIsInstance(self.model._standard_data, DNSTofDataset)
        self.model._standard_data = mock.Mock()
        self.model._standard_data.format_dataset = mock.Mock(
            return_value='test')
        self.model._standard_data.empty_filename = 'test_empty.d_dat'
        self.model._standard_data.vana_filename = 'test_vana.d_dat'

    def test_add_lines_to_script(self):
        self.model._script = []
        self.model._add_lines_to_script(['123', '456'])
        self.assertIsInstance(self.model._script, list)
        self.assertEqual(len(self.model._script), 2)
        self.assertEqual(self.model._script[0], '123')

    def test_script_maker(self):
        options = get_fake_tof_options()
        paths = get_paths()
        file_selector = get_file_selector_full_data()
        test_v = self.model.script_maker(options, paths, file_selector)
        self.assertIsInstance(test_v[0], list)
        self.assertEqual(len(test_v[0]), 50)
        self.assertEqual(test_v[1], '')
        for elm in test_v[0]:
            self.assertIsInstance(elm, str)
        options['dE_step'] = 0
        test_v = self.model.script_maker(options, paths, file_selector)
        self.assertEqual(test_v[1][:23], 'Binning makes no sense.')


if __name__ == '__main__':
    unittest.main()
