from mantid import FileFinder
from sans.gui_logic.models.run_file import RunFile


class RunFinder(object):
    def find_all_from_query(self, query_string):
        try:
            results = FileFinder.findRuns(query_string.encode('utf-8'))
            return ('', [RunFile(file_path) for file_path in results])
        except RuntimeError:
            return ('', [])
        except ValueError as ex:
            return (str(ex), [])

    def find_from_path(self, path):
        return RunFile(path)
