# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from sans.gui_logic.models.table_model import (TableModel, TableIndexModel, OptionsColumnModel, SampleShapeColumnModel, options_column_bool)
from sans.gui_logic.models.basic_hint_strategy import BasicHintStrategy
from qtpy.QtCore import QCoreApplication
from sans.common.enums import (RowState, SampleShape)


class TableModelTest(unittest.TestCase):
    def setUp(self):
        self.thickness_patcher = mock.patch('sans.gui_logic.models.table_model.create_file_information')
        self.addCleanup(self.thickness_patcher.stop)
        self.thickness_patcher.start()

    def test_user_file_can_be_set(self):
        self._do_test_file_setting(self._user_file_wrapper, "user_file")

    def test_batch_file_can_be_set(self):
        self._do_test_file_setting(self._batch_file_wrapper, "batch_file")

    def test_that_raises_if_table_index_does_not_exist(self):
        table_model = TableModel()
        row_entry = [''] * 16
        table_index_model = TableIndexModel(*row_entry)
        table_model.add_table_entry(0, table_index_model)
        self.assertRaises(IndexError, table_model.get_table_entry, 1)

    def test_that_can_get_table_index_model_for_valid_index(self):
        table_model = TableModel()
        row_entry = [''] * 16
        table_index_model = TableIndexModel(*row_entry)
        table_model.add_table_entry(0, table_index_model)
        returned_model = table_model.get_table_entry(0)
        self.assertEqual(returned_model.sample_scatter,  '')

    def test_that_can_set_the_options_column_model(self):
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "", "", "",
                                            options_column_string="WavelengthMin=1, WavelengthMax=3, NotRegister2=1")
        options_column_model = table_index_model.options_column_model
        options = options_column_model.get_options()
        self.assertEqual(len(options),  2)
        self.assertEqual(options["WavelengthMin"],  1.)
        self.assertEqual(options["WavelengthMax"],  3.)

    def test_that_raises_for_missing_equal(self):
        args = [0, "", "", "", "", "", "", "", "", "", "", "", "", "", ""]
        kwargs = {'options_column_string': "WavelengthMin=1, WavelengthMax=3, NotRegister2"}
        self.assertRaises(ValueError,  TableIndexModel, *args, **kwargs)

    def test_that_sample_shape_can_be_parsed(self):
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "", "", "",
                                            sample_shape="  flatPlate  ")
        sample_shape_enum = table_index_model.sample_shape
        sample_shape_text = table_index_model.sample_shape_string

        self.assertEqual(sample_shape_enum, SampleShape.FlatPlate)
        self.assertEqual(sample_shape_text, "FlatPlate")

    def test_that_sample_shape_can_be_set_as_enum(self):
        # If a batch file contains a sample shape, it is a enum: SampleShape.Disc, Cylinder, FlatPlate
        # So SampleShapeColumnModel must be able to parse this.
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "", "", "",
                                            sample_shape=SampleShape.FlatPlate)
        sample_shape_enum = table_index_model.sample_shape
        sample_shape_text = table_index_model.sample_shape_string

        self.assertEqual(sample_shape_enum, SampleShape.FlatPlate)
        self.assertEqual(sample_shape_text, "FlatPlate")

    def test_that_incorrect_sample_shape_reverts_to_previous_sampleshape(self):
        try:
            table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                                "", "", "", "", "", "", "", "",
                                                sample_shape="Disc")
            table_index_model.sample_shape = "not a sample shape"
        except Exception as e:
            self.fail("Did not except incorrect sample shape to raise error")
        else:
            self.assertEqual("Disc", table_index_model.sample_shape_string)

    def test_that_empty_string_is_acceptable_sample_shape(self):
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "", "", "",
                                            sample_shape="Disc")
        table_index_model.sample_shape = ""

        sample_shape_enum = table_index_model.sample_shape
        sample_shape_text = table_index_model.sample_shape_string

        self.assertEqual(sample_shape_enum, "")
        self.assertEqual(sample_shape_text, "")

    def test_that_table_model_completes_partial_sample_shape(self):
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "", "", "",
                                            sample_shape="cylind")

        sample_shape_enum = table_index_model.sample_shape
        sample_shape_text = table_index_model.sample_shape_string

        self.assertEqual(sample_shape_enum, SampleShape.Cylinder)
        self.assertEqual(sample_shape_text, "Cylinder")

    def test_that_querying_nonexistent_row_index_raises_IndexError_exception(self):
        table_model = TableModel()
        args = [0]
        self.assertRaises(IndexError, table_model.get_row_user_file, *args)

    def test_that_can_retrieve_user_file_from_table_index_model(self):
        table_model = TableModel()
        table_index_model = TableIndexModel('2', "", "", "", "", "", "",
                                            "", "", "", "", "", "", "User_file_name")
        table_model.add_table_entry(2, table_index_model)
        user_file = table_model.get_row_user_file(0)
        self.assertEqual(user_file,"User_file_name")

    def test_that_can_retrieve_sample_thickness_from_table_index_model(self):
        sample_thickness = '8.0'
        table_model = TableModel()
        table_index_model = TableIndexModel("", "", "", "", "", "",
                                            "", "", "", "", "", "", sample_thickness=sample_thickness)
        table_model.add_table_entry(2, table_index_model)
        row_entry = table_model.get_table_entry(0)
        self.assertEqual(row_entry.sample_thickness, sample_thickness)

    def test_that_parse_string_returns_correctly(self):
        string_to_parse = 'EventSlices=1-6,5-9,4:5:89 , WavelengthMax=78 , WavelengthMin=9'
        expected_dict = {'EventSlices': '1-6,5-9,4:5:89', 'WavelengthMax': '78', 'WavelengthMin': '9'}

        parsed_dict = OptionsColumnModel._parse_string(string_to_parse)

        self.assertEqual(parsed_dict, expected_dict)

    def test_get_number_of_rows_returns_number_of_entries(self):
        table_model = TableModel()
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        table_index_model = TableIndexModel('1', "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(1, table_index_model)

        number_of_rows = table_model.get_number_of_rows()

        self.assertEqual(number_of_rows, 2)

    def test_when_table_is_cleared_is_left_with_one_empty_row(self):
        table_model = TableModel()
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        table_index_model = TableIndexModel('1', "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(1, table_index_model)
        empty_row = table_model.create_empty_row()
        empty_row.id = 2

        table_model.clear_table_entries()

        self.assertEqual(table_model.get_number_of_rows(), 1)
        self.assertEqual(table_model.get_table_entry(0), empty_row)

    def test_when_last_row_is_removed_table_is_left_with_one_empty_row(self):
        table_model = TableModel()
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        table_index_model = TableIndexModel('1', "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(1, table_index_model)
        empty_row = table_model.create_empty_row()
        empty_row.id = 2

        table_model.remove_table_entries([0, 1])

        self.assertEqual(table_model.get_number_of_rows(), 1)
        self.assertEqual(table_model.get_table_entry(0), empty_row)

    def test_that_OptionsColumnModel_get_permissable_properties_returns_correct_properties(self):
        permissable_properties = OptionsColumnModel._get_permissible_properties()

        self.assertEqual(permissable_properties, {"WavelengthMin":float, "WavelengthMax": float, "EventSlices": str,
                                                  "MergeScale": float, "MergeShift": float, "PhiMin": float,
                                                  "PhiMax": float, "UseMirror": options_column_bool})

    def test_that_OptionsColumnModel_get_hint_strategy(self):
        hint_strategy = OptionsColumnModel.get_hint_strategy()
        expected_hint_strategy = BasicHintStrategy({
                                    "WavelengthMin": 'The min value of the wavelength when converting from TOF.',
                                    "WavelengthMax": 'The max value of the wavelength when converting from TOF.',
                                    "PhiMin": 'The min angle of the detector to accept.'
                                              ' Anti-clockwise from horizontal.',
                                    "PhiMax": 'The max angle of the detector to accept.'
                                              ' Anti-clockwise from horizontal.',
                                    "UseMirror": 'True or False. Whether or not to accept phi angle'
                                                 ' in opposing quadrant',
                                    "MergeScale": 'The scale applied to the HAB when merging',
                                    "MergeShift": 'The shift applied to the HAB when merging',
                                    "EventSlices": 'The event slices to reduce.'
                                                   ' The format is the same as for the event slices'
                                                   ' box in settings, however if a comma separated list is given '
                                                   'it must be enclosed in quotes'})

        self.assertEqual(expected_hint_strategy, hint_strategy)

    def test_that_row_state_is_initially_unprocessed(self):
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                            "", "", "", "", "", "")

        self.assertEqual(table_index_model.row_state, RowState.Unprocessed)
        self.assertEqual(table_index_model.tool_tip, '')

    def test_that_set_processed_sets_state_to_processed(self):
        table_model = TableModel()
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        row = 0
        tool_tip = 'Processesing succesful'

        table_model.set_row_to_processed(row, tool_tip)

        self.assertEqual(table_index_model.row_state, RowState.Processed)
        self.assertEqual(table_index_model.tool_tip, tool_tip)

    def test_that_reset_row_state_sets_row_to_unproceesed_and_sets_tool_tip_to_empty(self):
        table_model = TableModel()
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        row = 0
        tool_tip = 'Processesing succesful'
        table_model.set_row_to_processed(row, tool_tip)

        table_model.reset_row_state(row)

        self.assertEqual(table_index_model.row_state, RowState.Unprocessed)
        self.assertEqual(table_index_model.tool_tip, '')

    def test_that_set_row_to_error_sets_row_to_error_and_tool_tip(self):
        table_model = TableModel()
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        row = 0
        tool_tip = 'There was an error'

        table_model.set_row_to_error(row, tool_tip)

        self.assertEqual(table_index_model.row_state, RowState.Error)
        self.assertEqual(table_index_model.tool_tip, tool_tip)

    def test_serialise_options_dict_correctly(self):
        options_column_model = OptionsColumnModel('EventSlices=1-6,5-9,4:5:89 , WavelengthMax=78 , WavelengthMin=9')
        options_column_model.set_option('MergeScale', 1.5)

        options_string = options_column_model.get_options_string()

        self.assertEqual(options_string, 'EventSlices=1-6,5-9,4:5:89, MergeScale=1.5,'
                                         ' WavelengthMax=78.0, WavelengthMin=9.0')

    def test_that_truthy_options_are_evaluated_True(self):
        options_column_model = OptionsColumnModel('UseMirror=True')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': True})

        options_column_model = OptionsColumnModel('UseMirror=1')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': True})

        options_column_model = OptionsColumnModel('UseMirror=Yes')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': True})

        options_column_model = OptionsColumnModel('UseMirror=T')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': True})

        options_column_model = OptionsColumnModel('UseMirror=Y')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': True})

        options_column_model = OptionsColumnModel('UseMirror=tRuE')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': True})

    def test_that_falsy_options_are_evaluated_False(self):
        options_column_model = OptionsColumnModel('UseMirror=False')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': False})

        options_column_model = OptionsColumnModel('UseMirror=0')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': False})

        options_column_model = OptionsColumnModel('UseMirror=No')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': False})

        options_column_model = OptionsColumnModel('UseMirror=F')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': False})

        options_column_model = OptionsColumnModel('UseMirror=N')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': False})

        options_column_model = OptionsColumnModel('UseMirror=fAlSE')
        self.assertEqual(options_column_model.get_options(), {'UseMirror': False})

    def test_that_non_bool_option_raises_error_if_option_is_bool(self):
        with self.assertRaises(ValueError):
            OptionsColumnModel("UseMirror=SomeString")
            
    def test_that_to_batch_list_is_correct_format(self):
        test_row = ['SANS2D00022024  ', '', 'SANS2D00022025 ', '', '   SANS2D00022026 ', '', '', '', '', '', '', '',
                    '    out_file', 'a_user_file ', 1.0, 5.0, 5.4, 'Disc', 'WavelengthMax=5.0']
        table_index_model = TableIndexModel(*test_row)

        actual_list = table_index_model.to_batch_list()
        expected_list = ["SANS2D00022024", "SANS2D00022025", "SANS2D00022026",
                         "", "", "",  "out_file", "a_user_file", "1.0", "5.0", "5.4"]

        self.assertEqual(actual_list, expected_list)

    def test_that_get_non_empty_rows_returns_non_empty_rows(self):
        table_model = TableModel()
        table_index_model = TableIndexModel("", "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)
        table_index_model = TableIndexModel('0', "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(1, table_index_model)
        table_index_model = TableIndexModel('', "", "", "", "", "", "",
                                            "", "", "", "5", "", "")
        table_model.add_table_entry(2, table_index_model)
        table_index_model = TableIndexModel("", "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(3, table_index_model)

        non_empty_rows_actual = table_model.get_non_empty_rows([0, 1, 2, 3])
        non_empty_rows_expected = [1, 2]

        self.assertEqual(non_empty_rows_actual, non_empty_rows_expected)

    def _do_test_file_setting(self, func, prop):
        # Test that can set to empty string
        table_model = TableModel()
        try:
            setattr(table_model, prop, "")
            has_raised = False
        except:  # noqa
            has_raised = True
        self.assertFalse(has_raised)

        # Test that can be set to valid value
        setattr(table_model, prop, __file__)
        self.assertEqual(getattr(table_model, prop), __file__)

    @staticmethod
    def _batch_file_wrapper(value):
        table_model = TableModel()
        table_model.batch_file = value

    @staticmethod
    def _user_file_wrapper(value):
        table_model = TableModel()
        table_model.user_file = value


class TableModelThreadingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.qApp = QCoreApplication(['test_app'])

    @mock.patch('sans.gui_logic.presenter.create_file_information.SANSFileInformationFactory')
    def test_that_get_thickness_for_row_handles_errors_correctly(self, file_information_factory_mock):
        # self.thickness_patcher.stop()
        file_information_factory_instance = mock.MagicMock()
        file_information_factory_instance.create_sans_file_information.side_effect = RuntimeError('File Error')
        file_information_factory_mock.return_value = file_information_factory_instance
        table_model = TableModel()
        table_index_model = TableIndexModel("00000", "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)

        table_model.get_thickness_for_rows()
        table_model.work_handler.wait_for_done()
        self.qApp.processEvents()

        self.assertEqual(table_index_model.tool_tip, 'File Error')
        self.assertEqual(table_index_model.row_state, RowState.Error)

    def test_that_get_thickness_for_rows_updates_table_correctly(self):
        table_model = TableModel()
        table_index_model = TableIndexModel("LOQ74044", "", "", "", "", "", "",
                                            "", "", "", "", "", "")
        table_model.add_table_entry(0, table_index_model)

        table_model.get_thickness_for_rows()
        table_model.work_handler.wait_for_done()
        self.qApp.processEvents()

        self.assertEqual(table_index_model.sample_thickness, 1.0)


if __name__ == '__main__':
    unittest.main()
