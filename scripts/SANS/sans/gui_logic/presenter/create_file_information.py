from sans.gui_logic.presenter.work_handler_listener_wrapper import GenericWorkHandlerListener
from sans.common.file_information import SANSFileInformationFactory


def create_file_information(run_number, error_callback, success_callback, work_handler):
    listener = GenericWorkHandlerListener(error_callback, success_callback)
    file_information_factory = SANSFileInformationFactory()

    work_handler.process(listener, file_information_factory.create_sans_file_information, run_number)
