# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.sans_isis.work_handler import WorkHandler


class GenericWorkHandlerListener(WorkHandler.WorkListener):
    """
    A concrete class to act as a "listener" for the WorkHandler.
    """

    def __init__(self, error_callback, success_callback):
        """
        The callbacks are assigned to the abstract methods of the base class, and are called
        after certain actions of a running Worker (for example errors being raised or the
        Worker finishing its task).
        """
        super(GenericWorkHandlerListener, self).__init__()
        self.error_callback = error_callback
        self.success_callback = success_callback

    def on_processing_finished(self, result):
        self.success_callback(result)

    def on_processing_error(self, error):
        self.error_callback(error)
