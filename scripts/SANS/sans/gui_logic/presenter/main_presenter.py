"""  The main presenter.

The MainPresenter provides the QDataProcessorWidget with additional processing options which have not been
set on the data table. The MainPresenter is required by the DataProcessorWidget framework.
"""

from __future__ import (absolute_import, division, print_function)

from sans.gui_logic.presenter.run_tab_presenter import RunTabPresenter


class PresenterEnum(object):
    class RunTabPresenter(object):
        pass


class MainPresenter(object):
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

    This is probably no longer needed but leaving this layer in for now.
    """

    def __init__(self, facility):
        self._view = None

        self._facility = facility

        # Set of sub presenters
        self._presenters = {}
        self._presenters.update({PresenterEnum.RunTabPresenter: RunTabPresenter(facility=self._facility)})

    def set_view(self, view):
        self._view = view
        for _, presenter in self._presenters.items():
            presenter.set_view(self._view)
