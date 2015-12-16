#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.kernel import Direction, FloatBoundedValidator
import mantid.simpleapi
import os
import re

class SANSCopyUserIFleDependency(PythonAlgorithm):

    def category(self):
        return "SANS"

    def name(self):
        return "SANSCopyUserFileDependency"

    def summary(self):
        return "Reads all the dependencies from a user file and copies them to a designated folder."

    def PyInit(self):
        """
            Declare properties
        """
        self.declareProperty("Filename", "", direction = Direction.Input,
                             doc="Name of the user file.")
        self.declareProperty("OutputDirectory", "", direction = Direction.Input,
                             doc="The path to the output directory.")
        return

    def PyExec(self):
        user_file = self.getProperty("Filename").value
        output_dir = self.getProperty("OutputDirectory").value

        # Get the full user file path
        full_user_file_path = self._get_full_user_file_path(user_file)
        if full_user_file_path is None:
            pass

        # Make sure that output directory can be found
        if not self._does_directory_exist(output_dir):
            pass

        # Read all dependencies
        dependencies = self._get_all_dependencies(full_user_file_path)

        # Find all dependencies

        # Copy each file into the new directory

    def _get_full_user_file_path(self, user_file):
        user_file_path = None
        try:
            user_file_path = self._get_file_path(user_file)
        except:
            user_file_path = None
        return user_file_path

    def _does_directory_exist(self, directory):
        return os.path.isdir(directory)

    def _get_file_path(self, incomplete_path):
        this_path = FileFinder.getFullPath(incomplete_path)
        if not this_path:
            # do not catch exception, let it goes.
            this_path = FileFinder.findRuns(incomplete_path)
            # if list, get first value
            if hasattr(this_path, '__iter__'):
                this_path = this_path[0]

        return this_path

    def _get_all_dependencies(self, file_path):
        # The dependencies can have the following forms
        # 1. xxxx=Dependency.fileType as a direct assignment
        # 3. xxxx,Dependency.fileType as part of a list
        dependencies = []
        with open(file_path) as f:
            for line in f:
                dependencies_from_line = self._get_dependencies_from_line(line)
                dependencies.expand(dependencies_from_line)

    def _get_dependencies_from_line(self, line):
        file_types = [".nxs", ".xml", ".txt"]
        regexes_front = ["=[\s]*[\w]+", ",[\s]*[\w]+"]
        regex_collection = [front + back for front in regexes_front for back in file_types]



