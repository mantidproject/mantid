# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from sans.gui_logic.presenter.work_handler_listener_wrapper import GenericWorkHandlerListener
from sans.common.file_information import SANSFileInformationFactory


def create_file_information(run_number, error_callback, success_callback, work_handler, id):
    """
    The create_sans_file_information() is run in a new thread via the work_handler.

    :param run_number: Argument provided to create_sans_file_information.
    :param error_callback: Callback for error in create_sans_file_information.
    :param success_callback: Callback if create_sans_file_information succeeds.
    :param work_handler: WorkHandler instance.
    :param id: Identifier for processing.
    """
    listener = GenericWorkHandlerListener(error_callback, success_callback)
    file_information_factory = SANSFileInformationFactory()
    work_handler.process(listener,
                         file_information_factory.create_sans_file_information,
                         id,
                         run_number)
