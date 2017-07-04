from __future__ import (absolute_import, division, print_function)
import unittest
import mantid
from sans.test_helper.mock_objects import (MockRunTabView)
from sans.gui_logic.presenter.main_presenter import MainPresenter
from sans.common.enums import SANSFacility
from sans.test_helper.user_file_test_helper import (create_user_file, sample_user_file)
from sans.test_helper.common import (remove_file, save_to_csv)
from mantid.kernel import PropertyManagerDataService


class MainPresenterTest(unittest.TestCase):
    def test_that_gets_correct_gui_algorithm_name(self):
        presenter = MainPresenter(SANSFacility.ISIS)
        gui_algorithm_name = presenter.get_gui_algorithm_name()
        self.assertTrue(gui_algorithm_name == "SANSGuiDataProcessorAlgorithm")

    def that_populates_the_property_manager_data_service_when_processing_is_called(self):
        # Arrange
        self._clear_property_manager_data_service()
        presenter = MainPresenter(SANSFacility.ISIS)
        view = MockRunTabView()
        content = "# MANTID_BATCH_FILE add more text here\n" \
                  "sample_sans,SANS2D00022024,sample_trans,SANS2D00022048," \
                  "sample_direct_beam,SANS2D00022048,output_as,test_file\n" \
                  "sample_sans,SANS2D00022024,output_as,test_file2\n"
        batch_file_path = save_to_csv(content)
        view.set_batch_file_path(batch_file_path)
        user_file_path = create_user_file(sample_user_file)
        view.set_user_file_path(user_file_path)
        presenter.set_view(view)
        view.on_user_file_load()
        view.on_batch_file_load()

        # Act
        presenter.getProcessingOptions()

        # Assert
        # We should have two states in the PropertyManagerDataService
        self.assertTrue(len(PropertyManagerDataService.getObjectNames()) == 2)

        # clean up
        remove_file(sample_user_file)
        remove_file(user_file_path)

    def _clear_property_manager_data_service(self):
        for element in PropertyManagerDataService.getObjectNames():
            if PropertyManagerDataService.doesExist(element):
                PropertyManagerDataService.remove(element)


if __name__ == '__main__':
    unittest.main()


