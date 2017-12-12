class SummableRunFile(object):
    def __init__(self, path, run_number, is_event_mode):
        self._path = path
        self._is_event_mode = is_event_mode

    def is_event_data(self):
        return self._is_event_mode

    def file_path(self):
        return self._path

    def display_name(self):
        return self._display_name
