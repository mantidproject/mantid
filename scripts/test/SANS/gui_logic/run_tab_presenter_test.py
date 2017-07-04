from __future__ import (absolute_import, division, print_function)

import mantid
import unittest
import os
from sans.gui_logic.presenter.run_tab_presenter import RunTabPresenter
from sans.common.enums import (SANSFacility, ReductionDimensionality)
from sans.test_helper.user_file_test_helper import (create_user_file, sample_user_file)
from sans.test_helper.mock_objects import (MockRunTabView, MockStateModel)
from sans.test_helper.common import (remove_file, save_to_csv)


class RunTabPresenterTest(unittest.TestCase):
    def test_that_will_load_user_file(self):
        # Setup presenter and mock view
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = MockRunTabView()
        user_file_path = create_user_file(sample_user_file)
        view.set_user_file_path(user_file_path)
        presenter.set_view(view)

        # Act
        view.on_user_file_load()

        # Assert
        # Note that the event slices are not set in the user file
        self.assertTrue(not view.event_slices)
        self.assertTrue(view.reduction_dimensionality is ReductionDimensionality.OneDim)
        # Add future fields that will be populated here

        # clean up
        remove_file(user_file_path)

    def test_fails_silently_when_user_file_does_not_exist(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = MockRunTabView()
        view.set_user_file_path("non_existent_user_file")
        presenter.set_view(view)

        try:
            view.on_user_file_load()
            view.set_user_file_path("")
            view.on_user_file_load()
            has_raised = False
        except:
            has_raised = True
        self.assertFalse(has_raised)

    def test_that_loads_batch_file_and_places_it_into_table(self):
        # Arrange
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = MockRunTabView()
        content = "# MANTID_BATCH_FILE add more text here\n" \
                  "sample_sans,1,sample_trans,2,sample_direct_beam,3," \
                  "output_as,test_file,user_file,user_test_file\n" \
                  "sample_sans,1,can_sans,2,output_as,test_file2\n"
        batch_file_path = save_to_csv(content)
        view.set_batch_file_path(batch_file_path)
        presenter.set_view(view)

        # Act
        view.on_batch_file_load()

        # Assert
        rows = view.get_rows()
        self.assertTrue(len(rows) == 2)
        expected_first_row = "SampleScatter:1,SampleTransmission:2,SampleDirect:3," \
                             "CanScatter:,CanTransmission:,CanDirect:"
        self.assertTrue(expected_first_row == rows[0])
        expected_second_row = "SampleScatter:1,SampleTransmission:,SampleDirect:," \
                              "CanScatter:2,CanTransmission:,CanDirect:"
        self.assertTrue(expected_second_row == rows[1])

        # Clean up
        remove_file(batch_file_path)

    def test_fails_silently_when_batch_file_does_not_exist(self):
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = MockRunTabView()
        view.set_batch_file_path("non_existent_batch_file")
        presenter.set_view(view)

        try:
            view.on_batch_file_load()
            view.set_batch_file_path("")
            view.on_batch_file_load()
            has_raised = False
        except:
            has_raised = True
        self.assertFalse(has_raised)

    def test_that_gets_states_from_view(self):
        # Arrange
        presenter = RunTabPresenter(SANSFacility.ISIS)
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
        states = presenter.get_states()

        # Assert
        self.assertTrue(len(states) == 2)
        for _, state in states.items():
            try:
                state.validate()
                has_raised = False
            except:
                has_raised = True
            self.assertFalse(has_raised)

        # Check state 0
        state0 = states[0]
        self.assertTrue(state0.data.sample_scatter == "SANS2D00022024")
        self.assertTrue(state0.data.sample_transmission == "SANS2D00022048")
        self.assertTrue(state0.data.sample_direct == "SANS2D00022048")
        self.assertTrue(state0.data.can_scatter is None)
        self.assertTrue(state0.data.can_transmission is None)
        self.assertTrue(state0.data.can_direct is None)

        # Check state 1
        state1 = states[1]
        self.assertTrue(state1.data.sample_scatter == "SANS2D00022024")
        self.assertTrue(state1.data.sample_transmission is None)
        self.assertTrue(state1.data.sample_direct is None)
        self.assertTrue(state1.data.can_scatter is None)
        self.assertTrue(state1.data.can_transmission is None)
        self.assertTrue(state1.data.can_direct is None)

        # Check some entries
        self.assertTrue(state0.slice.start_time is None)
        self.assertTrue(state0.slice.end_time is None)
        self.assertTrue(state0.reduction.reduction_dimensionality is ReductionDimensionality.OneDim)

        # Clean up
        remove_file(batch_file_path)
        remove_file(user_file_path)

    def test_that_raises_runtime_error_if_states_are_not_valid(self):
        # Arrange
        presenter = RunTabPresenter(SANSFacility.ISIS)
        view = MockRunTabView()
        content = "# MANTID_BATCH_FILE add more text here\n" \
                  "sample_sans,SANS2D00022024,sample_trans,SANS2D00022048," \
                  "sample_direct_beam,SANS2D00022048,output_as,test_file\n" \
                  "sample_sans,SANS2D00022024,output_as,test_file2\n"
        batch_file_path = save_to_csv(content)
        view.set_batch_file_path(batch_file_path)
        presenter.set_view(view)
        presenter._state_model = MockStateModel()
        view.on_batch_file_load()

        # Act
        self.assertRaises(RuntimeError, presenter.get_states)

        # Clean up
        remove_file(batch_file_path)

    def test_that_can_get_state_for_index_if_index_exists(self):
        # Arrange
        presenter = RunTabPresenter(SANSFacility.ISIS)
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
        state = presenter.get_state_for_row(1)

        # Assert
        self.assertTrue(state.data.sample_scatter == "SANS2D00022024")
        self.assertTrue(state.data.sample_transmission is None)
        self.assertTrue(state.data.sample_direct is None)
        self.assertTrue(state.data.can_scatter is None)
        self.assertTrue(state.data.can_transmission is None)
        self.assertTrue(state.data.can_direct is None)

    def test_that_returns_none_when_index_does_not_exist(self):
        # Arrange
        presenter = RunTabPresenter(SANSFacility.ISIS)
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
        state = presenter.get_state_for_row(3)

        # Assert
        self.assertTrue(state is None)


if __name__ == '__main__':
    unittest.main()


