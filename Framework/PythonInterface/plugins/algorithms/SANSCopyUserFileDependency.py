#pylint: disable=no-init,invalid-name
from mantid.api import *
from mantid.kernel import Direction, FloatBoundedValidator
import mantid.simpleapi
import os
import re
import shutil

class SANSCopyUserFileDependency(PythonAlgorithm):

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
        dependency_extractor = UserFileDependencyExtractor()
        dependencies = dependency_extractor.get_all_dependencies(full_user_file_path)

        # Find all dependencies
        dependencies_full = self._find_all_dependencies(dependencies)

        # Get target names
        targets_full = self._get_targets(dependencies, output_dir)

        # Copy each file into the new directory
        self._copy_all_dependencies_to_new_directory(dependencies_full, targets_full)

    def _get_targets(self, dependencies, output_dir):
        targets = []
        for dependency in dependencies:
            targets.append(os.path.join(output_dir, dependency))
        return targets

    def _copy_all_dependencies_to_new_directory(self, dependencies_full, targets):
        '''
        @param dependencies_full: a list of full file paths to the dependencies
        @param depdendencies: a list of dependency names
        @param targets: a list of targets
        '''
        if len(dependencies_full) != len(targets):
            pass # TODO handle correctly

        soure_target = zip(dependencies_full, targets)

        for src, trg in soure_target:
            try:
                shutil.copyfile(src, trg)
            except IOError:
                pass # TODO handle correctlry here

    def _find_all_dependencies(self, dependencies):
        '''
        @param dependencies: a list of bare dependency names
        @return full paths to dependencies
        @throws RuntimeError: if a file cannot be found
        '''
        full_dependencies = []
        for dependency in dependencies:
            try:
                full_dependencies.append(self._get_file_path(dependency))
            except:
                #TODO Track error
                pass
        return full_dependencies

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


class UserFileDependencyExtractor(object):
    '''
    Extracts all depenency file names from a user file
    '''
    def __init__(self):
        super(UserFileDependencyExtractor, self).__init__()
        file_types = [".nxs", ".xml", ".txt"]
        regexes_front = ["=[\s]*[\w]+", ",[\s]*[\w]+"]
        self.regex_collection = [front + back for front in regexes_front for back in file_types]

    def get_all_dependencies(self, file_path):
        # The dependencies can have the following forms
        # 1. xxxx=Dependency.fileType as a direct assignment
        # 3. xxxx,Dependency.fileType as part of a list
        dependencies = []
        with open(file_path) as f:
            for line in f:
                dependencies_from_line = self._get_dependencies_from_line(line)
                dependencies.extend(dependencies_from_line)
        # Remove all commas, equal signs and whitespaces
        dependencies = [el.replace("=", "").replace(",","").replace(" ","") for el in dependencies]
        return set(dependencies)

    def _get_dependencies_from_line(self, line):
        found_items = []
        for element in self.regex_collection:
            found_items.extend(re.findall(element, line))
        return found_items

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(SANSCopyUserFileDependency)
