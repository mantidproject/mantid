from __future__ import (absolute_import, division, print_function)

import unittest

import mantid

from sans.test_helper.mock_objects import (create_mock_view2)
from sans.gui_logic.presenter.main_presenter import MainPresenter
from sans.common.enums import SANSFacility
from sans.test_helper.user_file_test_helper import (create_user_file, sample_user_file)
from sans.test_helper.common import (remove_file, save_to_csv)


class MainPresenterTest(unittest.TestCase):
    def test_that_gets_correct_gui_algorithm_name(self):
        presenter = MainPresenter(SANSFacility.ISIS)
        gui_algorithm_name = presenter.get_gui_algorithm_name()
        self.assertTrue(gui_algorithm_name == "SANSGuiDataProcessorAlgorithm")

    def test_that_the_white_list_is_correct(self):
        presenter = MainPresenter(SANSFacility.ISIS)
        self.assertTrue(presenter.get_number_of_white_list_items() == 0)
        white_list = presenter.get_white_list(show_periods=True)
        self.assertEqual(presenter.get_number_of_white_list_items(), 20)

        self.assertTrue(white_list[0].algorithm_property == "SampleScatter")
        self.assertTrue(white_list[1].algorithm_property == "SampleScatterPeriod")
        self.assertTrue(white_list[2].algorithm_property == "SampleTransmission")
        self.assertTrue(white_list[3].algorithm_property == "SampleTransmissionPeriod")
        self.assertTrue(white_list[4].algorithm_property == "SampleDirect")
        self.assertTrue(white_list[5].algorithm_property == "SampleDirectPeriod")
        self.assertTrue(white_list[6].algorithm_property == "CanScatter")
        self.assertTrue(white_list[7].algorithm_property == "CanScatterPeriod")
        self.assertTrue(white_list[8].algorithm_property == "CanTransmission")
        self.assertTrue(white_list[9].algorithm_property == "CanTransmissionPeriod")
        self.assertTrue(white_list[10].algorithm_property == "CanDirect")
        self.assertTrue(white_list[11].algorithm_property == "CanDirectPeriod")
        self.assertTrue(white_list[12].algorithm_property == "UseOptimizations")
        self.assertTrue(white_list[13].algorithm_property == "PlotResults")
        self.assertTrue(white_list[14].algorithm_property == "OutputName")
        self.assertTrue(white_list[15].algorithm_property == "UserFile")
        self.assertEqual(white_list[16].algorithm_property, "SampleThickness")
        self.assertTrue(white_list[17].algorithm_property == "RowIndex")
        self.assertTrue(white_list[18].algorithm_property == "OutputMode")
        self.assertTrue(white_list[19].algorithm_property == "OutputGraph")

    def test_that_black_list_is_correct(self):
        presenter = MainPresenter(SANSFacility.ISIS)
        expected = "InputWorkspace,OutputWorkspace,SampleScatter,SampleScatterPeriod,SampleTransmission," \
                   "SampleTransmissionPeriod,SampleDirect,SampleDirectPeriod,CanScatter,CanScatterPeriod," \
                   "CanTransmission,CanTransmissionPeriod,CanDirect,CanDirectPeriod," \
                   "UseOptimizations,PlotResults,OutputName,UserFile,SampleThickness,RowIndex,OutputMode,OutputGraph,"
        self.assertEqual(expected, presenter.get_black_list())

    def test_that_gets_pre_processing_options_are_valid_and_other_options_are_empty(self):
        # Arrange
        presenter = MainPresenter(SANSFacility.ISIS)
        content = "# MANTID_BATCH_FILE add more text here\n" \
                  "sample_sans,SANS2D00022024,sample_trans,SANS2D00022048," \
                  "sample_direct_beam,SANS2D00022048,output_as,test_file\n" \
                  "sample_sans,SANS2D00022024,output_as,test_file2\n"
        batch_file_path = save_to_csv(content)
        user_file_path = create_user_file(sample_user_file)
        view = create_mock_view2(user_file_path, batch_file_path)
        presenter.set_view(view)

        # Act
        pre_processing_options = presenter.getProcessingOptions()

        # Assert
        expected = {'UseOptimizations':'1','OutputMode':'PublishToADS','PlotResults':'1', \
                    'OutputGraph':'SANS-Latest'}
        self.assertEqual(expected, pre_processing_options)
        self.assertFalse(presenter.getPreprocessingOptions())
        self.assertFalse(presenter.getPostprocessingOptionsAsString())

        # Clean up
        remove_file(sample_user_file)
        remove_file(user_file_path)


if __name__ == '__main__':
    unittest.main()


