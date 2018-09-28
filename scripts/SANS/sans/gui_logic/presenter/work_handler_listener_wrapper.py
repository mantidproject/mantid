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