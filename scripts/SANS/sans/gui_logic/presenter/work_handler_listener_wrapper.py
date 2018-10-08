# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from ui.sans_isis.work_handler import WorkHandler


class GenericWorkHandlerListener(WorkHandler.WorkListener):
    def __init__(self, error_callback, success_callback):
        super(GenericWorkHandlerListener, self).__init__()
        self.error_callback = error_callback
        self.success_callback = success_callback

    def on_processing_finished(self, result):
        self.success_callback(result)

    def on_processing_error(self, error):
        self.error_callback(error)
