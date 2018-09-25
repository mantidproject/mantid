from __future__ import (absolute_import, division, print_function)


class HelpWidgetPresenter(object):

    def __init__(self, view, model):
        self._view = view
        self._model = model

        self._view.on_manage_user_directories_clicked(self.handle_manage_user_directories)
        self._view.on_help_button_clicked(self.handle_help_button_clicked)

    def handle_manage_user_directories(self):
        self._view.show_directory_manager()

    def handle_help_button_clicked(self):
        self._view.warning_popup("Help is not currently implemented!")
