# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from sans.gui_logic.presenter.work_handler_listener_wrapper import GenericWorkHandlerListener
from sans.common.file_information import SANSFileInformationFactory


def create_file_information(run_number, error_callback, success_callback, work_handler, id):
    listener = GenericWorkHandlerListener(error_callback, success_callback)
    file_information_factory = SANSFileInformationFactory()

    work_handler.process(listener, file_information_factory.create_sans_file_information, id, run_number)
