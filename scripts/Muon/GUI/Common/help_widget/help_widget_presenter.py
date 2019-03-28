from __future__ import (absolute_import, division, print_function)
from Muon.GUI.Common.help_widget.help_widget_view import HelpWidgetView


class HelpWidgetPresenter(object):

    def __init__(self, view):
        self._view = view

        self._view.on_manage_user_directories_clicked(self.handle_manage_user_directories)
        self._view.on_help_button_clicked(self.handle_help_button_clicked)

    def handle_manage_user_directories(self):
        self._view.show_directory_manager()

    def handle_help_button_clicked(self):
        self._view._on_help_button_clicked()


class HelpWidget(object):
    def __init__(self):
        self.view = HelpWidgetView()
        self.presenter = HelpWidgetPresenter(self.view)
