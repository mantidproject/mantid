"""  The main presenter.

The MainPresenter provides the QDataProcessorWidget with additional processing options which have not been
set on the data table. The MainPresenter is required by the DataProcessorWidget framework.
"""

from __future__ import (absolute_import, division, print_function)

from mantidqtpython import MantidQt

from sans.gui_logic.sans_data_processor_gui_algorithm import (get_gui_algorithm_name, get_white_list,
                                                              get_black_list)
from sans.gui_logic.presenter.run_tab_presenter import RunTabPresenter


class PresenterEnum(object):
    class RunTabPresenter(object):
        pass


class MainPresenter(MantidQt.MantidWidgets.DataProcessor.DataProcessorMainPresenter):
    """
    Comments below are from Raquel:

    A DataProcessorMainPresenter. The base class provides default implementations
    but we should re-implement the following methods:
    - getPreprocessingOptions() -- to supply global pre-processing options to the table widget
    - getProcessingOptions() -- to supply global processing options
    - getPostprocessingOptionsAsString() -- to supply global post-processing options
    - notifyADSChanged() -- to act when the ADS changed, typically we want to update
      table actions with the list of table workspaces that can be loaded into the interface

    This is an intermediate layer needed in python. Ideally our gui class should
    inherit from 'DataProcessorMainPresenter' directly and provide the required implementations,
    but multiple inheritance does not seem to be fully supported, hence we need this extra class.
    """

    def __init__(self, facility):
        super(MantidQt.MantidWidgets.DataProcessor.DataProcessorMainPresenter, self).__init__()

        self._view = None

        # Algorithm details
        self._gui_algorithm_name = None
        self._white_list = None
        self._black_list = None
        self._facility = facility

        # Set of sub presenters
        self._presenters = {}
        self._presenters.update({PresenterEnum.RunTabPresenter: RunTabPresenter(facility=self._facility)})

    def set_view(self, view):
        self._view = view
        for _, presenter in self._presenters.items():
            presenter.set_view(self._view)

    def get_gui_algorithm_name(self):
        if self._gui_algorithm_name is None:
            self._gui_algorithm_name = get_gui_algorithm_name(self._facility)
        return self._gui_algorithm_name

    def get_white_list(self, show_periods=False):
        self._white_list = get_white_list(show_periods=show_periods)
        return self._white_list

    def get_number_of_white_list_items(self):
        return 0 if not self._white_list else len(self._white_list)

    def get_black_list(self):
        if self._black_list is None:
            self._black_list = get_black_list()
        return self._black_list

    def confirmReductionPaused(self, group):
        self._presenters[PresenterEnum.RunTabPresenter].on_processing_finished()

    # ------------------------------------------------------------------------------------------------------------------
    # Inherited methods
    # ------------------------------------------------------------------------------------------------------------------
    def getProcessingOptions(self):
        """
        Gets the processing options from the run tab presenter
        """
        return self._presenters[PresenterEnum.RunTabPresenter].get_processing_options()

    # ------------------------------------------------------------------------------------------------------------------
    # Unused
    # ------------------------------------------------------------------------------------------------------------------
    def getPreprocessingOptions(self):
        empty = {}
        return empty

    def getPostprocessingOptionsAsString(self):
        return ""

    def notifyADSChanged(self, workspace_list):
        self._view.add_actions_to_menus(workspace_list)
