# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import os

from qtpy.QtWidgets import QApplication, QFileDialog, QMessageBox

from mantid.api import AnalysisDataService, AnalysisDataServiceObserver, WorkspaceGroup
from mantid.kernel import ConfigService
from mantid.plots import MantidAxes
from mantidqt.io import open_a_file_dialog
from mantidqt.project.projectloader import ProjectLoader
from mantidqt.project.projectsaver import ProjectSaver
from mantidqt.utils.asynchronous import BlockingAsyncTaskWithCallback
from mantidqt.widgets.saveprojectdialog.presenter import ProjectSaveDialogPresenter
from mantidqt.utils.qt.qappthreadcall import QAppThreadCall


class Project(AnalysisDataServiceObserver):
    def __init__(self, globalfiguremanager_instance, interface_populating_function):
        """
        :param globalfiguremanager_instance: The global figure manager instance used in this project.
        :param interface_populating_function: The interface populating function which returns a list of lists of windows
         and encoders
        """
        super(Project, self).__init__()
        # Has the project been saved, to Access this call .saved
        self.__saved = True

        self.__is_saving = False
        self.__is_loading = False

        # Last save locations
        self.last_project_location = None

        # whether to discard workspaces that have only been loaded when saving the project
        self.save_altered_workspaces_only = False

        self.observeAll(True)

        self.project_file_ext = ".mtdproj"
        self.mplot_project_file_ext = ".mantid"
        self.valid_file_exts = [self.project_file_ext, self.mplot_project_file_ext]

        self.plot_gfm = globalfiguremanager_instance
        self.plot_gfm.add_observer(self)

        self.interface_populating_function = interface_populating_function

        self.prompt_save_on_close = True

    def load_settings_from_config(self, config):
        self.prompt_save_on_close = config.get("project", "prompt_save_on_close", type=bool)
        self.save_altered_workspaces_only = config.get("project", "save_altered_workspaces_only", type=bool)

    @property
    def saved(self):
        return self.__saved

    @property
    def is_saving(self):
        return self.__is_saving

    @property
    def is_loading(self):
        return self.__is_loading

    def save(self, conf=None):
        """
        The function that is called if the save button is clicked on the mainwindow
        :param: conf: an optional UserConfig to which the user's input will be saved (if the save dialog is shown).
        :return: True; if the user cancels
        """
        if self.last_project_location is None:
            return self.open_project_save_dialog(conf)
        else:
            # Offer an are you sure? overwriting GUI
            answer = self._offer_overwriting_gui()

            if answer == QMessageBox.Yes:
                # Actually save
                task = BlockingAsyncTaskWithCallback(target=self._save, blocking_cb=QApplication.processEvents)
                task.start()
            elif answer == QMessageBox.No:
                # Save with a new name
                return self.open_project_save_dialog(conf)
            else:
                # Cancel clicked
                return True

    def open_project_save_dialog(self, conf=None):
        """
        The function that is called if the save as... button is clicked on the mainwindow
        :param: conf: an optional UserConfig to which the user's input will be saved.
        :return: True; if the user cancels.
        """
        self.saving_cancelled = False
        ProjectSaveDialogPresenter(self, conf)
        return self.saving_cancelled

    def save_as(self, path):
        self.last_project_location = path
        task = BlockingAsyncTaskWithCallback(target=self._save, blocking_cb=QApplication.processEvents)
        task.start()

    @staticmethod
    def _offer_overwriting_gui():
        """
        Offers up a overwriting QMessageBox giving the option to overwrite a project, and returns the reply.
        :return: QMessaageBox.Yes or QMessageBox.No or QMessageBox.Cancel; The value is the value selected by the user.
        """
        return QMessageBox().question(
            None,
            "Overwrite project?",
            "Would you like to overwrite the selected project?",
            QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
            QMessageBox.Yes,
        )

    def _save_file_dialog(self):
        return open_a_file_dialog(
            accept_mode=QFileDialog.AcceptSave,
            file_mode=QFileDialog.AnyFile,
            file_filter="Project files ( *" + self.project_file_ext + ")",
            directory=ConfigService["defaultsave.directory"],
        )

    def _save(self):
        self.__is_saving = True
        try:
            workspaces_to_save = self._get_workspace_names_to_save()

            if self.save_altered_workspaces_only:
                workspaces_to_save = self._filter_unaltered_workspaces(workspaces_to_save)

            # Calculate the size of the workspaces in the project.
            project_size = self._get_project_size(workspaces_to_save)
            warning_size = int(ConfigService.getString("projectSaving.warningSize"))
            # If a project is > the value in the properties file, question the user if they want to continue.
            result = None
            if project_size > warning_size:
                # we have to create the message box in the main thread
                result = QAppThreadCall(self._offer_large_size_confirmation)()

            if result is None or result != QMessageBox.Cancel:
                plots_to_save = self.plot_gfm.figs

                if self.save_altered_workspaces_only:
                    plots_to_save = self._filter_plots_with_unaltered_workspaces(plots_to_save, workspaces_to_save)

                interfaces_to_save = self.interface_populating_function()
                project_saver = ProjectSaver(self.project_file_ext)
                project_saver.save_project(
                    file_name=self.last_project_location,
                    workspace_to_save=workspaces_to_save,
                    plots_to_save=plots_to_save,
                    interfaces_to_save=interfaces_to_save,
                )
                self.__saved = True
        finally:
            self.__is_saving = False

    @staticmethod
    def _get_workspace_names_to_save():
        """
        Get a list of the names of top level workspaces (workspaces not in groups) by getting every workspace,
        checking if the workspace is in a group, and removing it from the full list.
        :return: A list of the top level workspaces in the ADS
        """
        workspace_names = AnalysisDataService.getObjectNames()
        workspaces_in_ads = AnalysisDataService.retrieveWorkspaces(workspace_names)
        group_workspaces = [workspace for workspace in workspaces_in_ads if isinstance(workspace, WorkspaceGroup)]
        workspaces_in_groups_names = []
        for group_ws in group_workspaces:
            workspaces_in_groups_names.extend(group_ws.getNames())
        return [name for name in workspace_names if name not in workspaces_in_groups_names]

    @staticmethod
    def _filter_unaltered_workspaces(workspace_names):
        """
        Removes workspaces whose history contains Load and nothing else.
        :param workspace_names: a list of workspace names
        :return: the filtered list of workspace names.
        """
        workspaces = AnalysisDataService.retrieveWorkspaces(workspace_names)
        altered_workspace_names = []
        for ws in workspaces:
            history = ws.getHistory()
            if not (history.size() == 1 and history.getAlgorithm(0).name() == "Load"):
                altered_workspace_names.append(ws.name())

        return altered_workspace_names

    @staticmethod
    def _filter_plots_with_unaltered_workspaces(plots, workspaces):
        """
        :param plots: a dictionary of figure managers.
        :param workspaces: a list of workspace names.
        :return: a dictionary of figure managers containing plots that only use the workspaces in workspaces.
        """
        plots_copy = plots.copy()
        for i, plot in plots_copy.items():
            # check that every axes only uses workspaces that are being saved, otherwise delete the plot
            if not all(
                all(ws in workspaces for ws in ax.tracked_workspaces if isinstance(ax, MantidAxes)) for ax in plot.canvas.figure.axes
            ):
                del plots[i]

        return plots

    @staticmethod
    def inform_user_not_possible():
        return QMessageBox().information(
            None, "That action is not possible!", "You cannot exit workbench whilst it is saving or loading a project"
        )

    @staticmethod
    def _get_project_size(workspace_names):
        project_size = 0
        for name in workspace_names:
            project_size += AnalysisDataService.retrieve(name).getMemorySize()
        return project_size

    def load(self):
        """
        The event that is called when open project is clicked on the main window
        :return: None; if the user cancelled.
        """
        self.__is_loading = True
        try:
            file_name = self._load_file_dialog()
            if file_name is None:
                # Cancel close dialogs
                return

            # Sanity check
            _, file_ext = os.path.splitext(file_name)

            if file_ext not in self.valid_file_exts:
                QMessageBox.warning(None, "Wrong file type!", "Please select a valid project file", QMessageBox.Ok)

            self._load(file_name)

            self.last_project_location = file_name
            self.__saved = True
        finally:
            self.__is_loading = False

    def _load(self, file_name):
        project_loader = ProjectLoader(self.project_file_ext)
        task = BlockingAsyncTaskWithCallback(target=project_loader.load_project, args=[file_name], blocking_cb=QApplication.processEvents)
        task.start()

    def _load_file_dialog(self):
        return open_a_file_dialog(
            accept_mode=QFileDialog.AcceptOpen,
            file_mode=QFileDialog.ExistingFile,
            file_filter="Project files ( *" + " *".join(self.valid_file_exts) + ")",
            directory=ConfigService["defaultsave.directory"],
        )

    def offer_save(self, parent):
        """
        :param parent: QWidget; Parent of the QMessageBox that is popped up
        :return: Bool; Returns false if no save needed/save complete. Returns True if need to cancel closing. However
                        will return None if self.__saved is false
        """
        # If the current project is saved then return and don't do anything
        if self.__saved:
            return

        result = self._offer_save_message_box(parent)

        if result == QMessageBox.Yes:
            if self.save():
                return True
        elif result == QMessageBox.Cancel:
            return True
        # if yes or no return false
        return False

    def _offer_save_message_box(self, parent):
        if self.prompt_save_on_close:
            return QMessageBox.question(
                parent,
                "Unsaved Project",
                "The project is currently unsaved. Would you like to save before closing?",
                QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                QMessageBox.Yes,
            )
        else:
            return QMessageBox.No

    @staticmethod
    def _offer_large_size_confirmation():
        """
        Asks the user to confirm that they want to save a large project.
        :return: QMessageBox; The response from the user. Default is Yes.
        """
        return QMessageBox.question(
            None,
            "You are trying to save a large project.",
            "The project may take a long time to save. Would you like to continue?",
            QMessageBox.Yes | QMessageBox.Cancel,
            QMessageBox.Cancel,
        )

    def modified_project(self):
        self.__saved = False

    def anyChangeHandle(self):
        """
        The method that will be triggered if any of the changes in the ADS have occurred, that are checked for using the
        AnalysisDataServiceObserver class' observeAll method
        """
        self.modified_project()

    def notify(self, *args):
        """
        The method that will trigger when a plot is added, destroyed, or changed in the global figure manager.
        """
        self.modified_project()
