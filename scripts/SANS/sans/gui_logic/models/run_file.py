class SummableRunFile(object):
    def __init__(self, path, run_number, is_event_mode):
        assert(type(path) == str)
        assert(type(run_number) == str)

        self._path = path
        self._run_number = run_number
        self._is_event_mode = is_event_mode

    def is_event_data(self):
        return self._is_event_mode

    def file_path(self):
        return self._path

    def display_name(self):
        return str(self._run_number)
