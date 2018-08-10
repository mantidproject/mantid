import sys
import time


from Muon.GUI.MuonAnalysis.loadfile.load_file_view import BrowseFileWidgetView
from Muon.GUI.MuonAnalysis.loadfile.load_file_presenter import BrowseFileWidgetPresenter
from Muon.GUI.MuonAnalysis.loadfile.load_file_model import BrowseFileWidgetModel

import unittest

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class LoadFileWidgetModelTest(unittest.TestCase):

    def test_model_initialized_with_empty_lists_of_loaded_data(self):
        model = BrowseFileWidgetModel()
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])

    def test_executing_load_without_filenames_does_nothing(self):
        model = BrowseFileWidgetModel()
        model.execute()
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])

    def test_executing_load_with_fully_qualified_filename_loads_correctly(self):
        # test >1 instrument, >1 os
        model = BrowseFileWidgetModel()
        files = [r'C:\Users\zjq85562\Dropbox\Mantid-RAL\Testing\TrainingCourseData\muon_cupper\EMU00020889.nxs',
                 '\\\\EMU\\data\\EMU00083416.nxs']
        model._filenames = files  # ["\\\\EMU\\data\\EMU00083416.nxs"]
        model.execute()
        self.assertEqual(len(model.loaded_workspaces), len(model.loaded_runs))
        self.assertEqual(len(model.loaded_workspaces), len(model.loaded_filenames))
        self.assertEqual(len(model.loaded_workspaces), 2)
        self.assertEqual(model.loaded_filenames[0], files[0])
        self.assertEqual(model.loaded_runs[0], 20889)

    # def test_executing_filename_only_loads_correctly_from_network(self):
    #     model = BrowseFileWidgetModel()
    #     files = [r'EMU03087.nxs']
    #     model._filenames = files
    #     model.execute()
    #     self.assertNotEqual(model.loaded_filenames[0], files[0])
    #     self.assertEqual(model.loaded_runs[0], 83416)

    def test_model_is_cleared_correctly(self):
        model = BrowseFileWidgetModel()
        model._filenames = ['\\\\EMU\\data\\EMU00083416.nxs']
        model.execute()
        model.clear()
        self.assertEqual(model.loaded_workspaces, [])
        self.assertEqual(model.loaded_filenames, [])
        self.assertEqual(model.loaded_runs, [])


def wait_for_thread(thread, timeout=10):
    start = time.time()
    while (time.time() - start < timeout):
        time.sleep(0.1)
        if thread.isFinished():
            return True
    return False


class LoadFileWidgetPresenterTest(unittest.TestCase):

    def setUp(self):
        self.mock_view = mock.create_autospec(BrowseFileWidgetView, spec_set=False)

        # self.mock_view = BrowseFileWidgetView()
        self.mock_view.on_browse_clicked = mock.Mock()
        self.mock_view.set_file_edit = mock.Mock()
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        self.mock_model = mock.create_autospec(BrowseFileWidgetModel, spec_set=True)
        self.mock_model.execute = mock.Mock()
        self.mock_model.exception_message_for_failed_files = mock.Mock()

        self.mock_view.disable_load_buttons = mock.Mock()
        self.mock_view.enable_load_buttons = mock.Mock()

        # self.presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)

    def test_dialog_opens_when_browse_button_clicked(self):
        presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)
        presenter.on_browse_button_clicked()

        self.mock_view.show_file_browser_and_return_selection.assert_called_once()

    def test_buttons_disabled_while_load_thread_running(self):
        presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)
        presenter.on_browse_button_clicked()

        self.mock_view.disable_load_buttons.assert_called_once()
        self.mock_view.enable_load_buttons.assert_called_once()

    def test_buttons_enabled_if_load_thread_throws(self):
        def load_failure():
            raise ValueError("Error text")

        self.mock_model.execute = mock.Mock(side_effect=load_failure)

        presenter = BrowseFileWidgetPresenter(self.mock_view, self.mock_model)
        presenter.on_browse_button_clicked()

        self.mock_view.disable_load_buttons.assert_called_once()
        self.mock_view.enable_load_buttons.assert_called_once()

    def test_buttons_enabled_even_if_load_thread_throws(self):
        model = BrowseFileWidgetModel()
        presenter = BrowseFileWidgetPresenter(self.mock_view, model)

        presenter.on_browse_button_clicked()

        self.mock_view.disable_load_buttons.assert_called_once()
        self.mock_view.enable_load_buttons.assert_called_once()

    def test_files_loaded_into_model_from_browse_selection(self):
        presenter = BrowseFileWidgetPresenter(BrowseFileWidgetView(), BrowseFileWidgetModel())
        presenter.on_browse_button_clicked()

    def test_files_loaded_into_model_from_browse_selection(self):
        model = BrowseFileWidgetModel()
        model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3, 4], 1234))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        presenter = BrowseFileWidgetPresenter(self.mock_view, model)

        presenter.on_browse_button_clicked()
        #assert (wait_for_thread(presenter._load_thread))
        assert (presenter._load_thread.wait())

        print(model.loaded_workspaces)
        print(model.loaded_runs)
        self.assertEqual(len(model.loaded_filenames), 2)
        self.assertEqual(len(model.loaded_workspaces), 2)
        self.assertEqual(len(model.loaded_runs), 2)

    def test_files_not_loaded_into_model_if_multiple_files_selected_in_single_file_mode(self):
        model = BrowseFileWidgetModel()
        model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3, 4], 1234))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs", "C:/dir2/file2.nxs"])

        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(False)

        self.assertRaises(ValueError, presenter.on_browse_button_clicked)
        self.mock_view.disable_load_buttons.assert_not_called()
        self.mock_view.enable_load_buttons.assert_not_called()

    def test_single_file_loaded_into_model_in_single_file_mode(self):
        model = BrowseFileWidgetModel()
        view = BrowseFileWidgetView()



        model.load_workspace_from_filename = mock.Mock(return_value=([1, 2, 3, 4], 1234))
        self.mock_view.show_file_browser_and_return_selection = mock.Mock(
            return_value=["C:/dir1/file1.nxs"])

        self.file_edit = ""
        self.mock_view._stored_text = ""
        def set_local(*args):
            self._stored_text = args[0]
        self.mock_view.set_file_edit = mock.Mock(side_effect = set_local)

        presenter = BrowseFileWidgetPresenter(self.mock_view, model)
        presenter.enable_multiple_files(False)

        presenter.on_browse_button_clicked()
        assert (presenter._load_thread.wait())

        self.mock_view.disable_load_buttons.assert_called_once()
        self.mock_view.enable_load_buttons.assert_called_once()
        self.assertEqual(model.loaded_filenames, ["C:/dir1/file1.nxs"])
        self.assertEqual(model.loaded_workspaces, [[1, 2, 3, 4]])
        self.assertEqual(model.loaded_runs, [1234])

        # check the view
        #self.mock_view.set_file_edit("hello")
        print("file edit : " + self.file_edit)
        print("stored text : " + self.mock_view._stored_text)
        #self.mock_view.set_file_edit.assert_any_call("C:/dir1/file1.nxs")



#
#     def test_execute(self):
#         empty = {}
#         inputs={}
#         inputs["Run"]=empty
#         #inputs["phaseTable"]=None
#         inputs["preRe"]=empty
#         #inputs["preIm"]=None
#         inputs["FFT"]=empty
#         self.wrapper.loadData(inputs)
#         self.wrapper.execute()
#         assert(self.model.setRun.call_count==1)
#         assert(self.model.preAlg.call_count==1)
#         assert(self.model.FFTAlg.call_count==1)
#         assert(self.model.makePhaseQuadTable.call_count==0)
#         assert(self.model.PhaseQuad.call_count==0)


if __name__ == '__main__':
    unittest.main()
