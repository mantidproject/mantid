import os

class RunFile(object):
    def __init__(self, path):
        self._path = path
        self._display_name = RunFile._path_to_display_name(path)

    def file_path(self):
        return self._path

    def display_name(self):
        return self._display_name

    @staticmethod
    def _path_to_display_name(path):
        return os.path.splitext(os.path.basename(path))[0]
