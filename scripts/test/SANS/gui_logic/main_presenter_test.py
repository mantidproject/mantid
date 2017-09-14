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
        white_list = presenter.get_white_list()
        self.assertTrue(presenter.get_number_of_white_list_items() == 10)
        self.assertTrue(white_list[0].algorithm_property == "SampleScatter")
        self.assertTrue(white_list[1].algorithm_property == "SampleTransmission")
        self.assertTrue(white_list[2].algorithm_property == "SampleDirect")
        self.assertTrue(white_list[3].algorithm_property == "CanScatter")
        self.assertTrue(white_list[4].algorithm_property == "CanTransmission")
        self.assertTrue(white_list[5].algorithm_property == "CanDirect")
        self.assertTrue(white_list[6].algorithm_property == "UseOptimizations")
        self.assertTrue(white_list[7].algorithm_property == "OutputName")
        self.assertTrue(white_list[8].algorithm_property == "RowIndex")
        self.assertTrue(white_list[9].algorithm_property == "OutputMode")

    def test_that_black_list_is_correct(self):
        presenter = MainPresenter(SANSFacility.ISIS)
        expected = "InputWorkspace,OutputWorkspace,SampleScatter,SampleTransmission,SampleDirect,CanScatter," \
                   "CanTransmission,CanDirect,UseOptimizations,OutputName,RowIndex,OutputMode,"
        self.assertTrue(expected == presenter.get_black_list())

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
        expected = "UseOptimizations=1,OutputMode=PublishToADS"
        self.assertTrue(expected == pre_processing_options)
        self.assertFalse(presenter.getPreprocessingOptionsAsString())
        self.assertFalse(presenter.getPostprocessingOptions())

        # Clean up
        remove_file(sample_user_file)
        remove_file(user_file_path)


if __name__ == '__main__':
    unittest.main()


