#pylint: disable=no-init,invalid-name
from mantid.api import *
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
        self.declareProperty(FileProperty(name = "Filename", defaultValue = "",
                                          action = FileAction.Load, extensions = ["*.txt"]))
        self.declareProperty(FileProperty(name = "OutputDirectory", defaultValue = "",
                                          action = FileAction.Directory),
                             doc="The path to the output directory.")

    def PyExec(self):
        user_file = self.getProperty("Filename").value
        output_dir = self.getProperty("OutputDirectory").value

        # Get the full user file path
        full_user_file_path = self._get_full_user_file_path(user_file)

        # Make sure that output directory can be found
        self._check_if_directory_exists(output_dir)

        # Read all dependencies
        dependency_extractor = UserFileDependencyExtractor()
        dependencies = dependency_extractor.get_all_dependencies(full_user_file_path)

        # Find all dependencies
        dependencies_full = self._find_all_dependencies(dependencies)

        # Get target names
        targets_full = self._get_dependency_targets(dependencies, output_dir)

        # Copy each file into the new directory
        self._copy_all_dependencies_to_new_directory(dependencies_full, targets_full)

        # Finally copy the user file itself into the target directory
        self._copy_user_file_to_target_directory(full_user_file_path , output_dir, targets_full)

    def _copy_user_file_to_target_directory(self, src, target_directory, dependencies):
        trg = self._get_user_file_target(src, target_directory)
        try:
            shutil.copyfile(src, trg)
        except IOError:
            self._remove_copied_files(dependencies)
            error_msg = ("SANSCopyUserFileDependency: There was an issue copying"
                         " the file " + src + " to " + trg + ". Attempted to remove all"
                         "copied files. Please make sure that you have write permissions.")
            raise RuntimeError(error_msg)

    def _get_user_file_target(self, user_file_full_path, target_directory):
        basename = os.path.basename(user_file_full_path)
        target = os.path.join(target_directory, basename)
        return target

    def _get_dependency_targets(self, dependencies, output_dir):
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
            error_msg = ("SANSCopyUserFileDependency: There was a mismatch between the number "
                         " of source and target dependencies.")
            raise RuntimeError(error_msg)

        source_target = zip(dependencies_full, targets)
        index = 0
        for src, trg in source_target:
            try:
                shutil.copyfile(src, trg)
                index += 1
            except IOError:
                dummy_source, remove_targets = zip(*source_target[0:index])
                remove_targets = list(remove_targets)
                self._remove_copied_files(remove_targets)
                error_msg = ("SANSCopyUserFileDependency: There was an issue copying"
                             " the file " + src + " to " + trg + ". Attempted to remove all"
                             "copied files. Please make sure that you have write permissions.")
                raise RuntimeError(error_msg)

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
                error_msg = ("SANSCopyUserFileDependency: The dependency " + str(dependency) +
                             " could not be found. Make sure it is in the Mantid path.")
                raise RuntimeError(error_msg)
        return full_dependencies

    def _get_full_user_file_path(self, user_file):
        '''
        @param user_file: the name or full path to the user file
        @returns the full file name
        @raises RuntimeError: if the file cannot be found.
        '''
        user_file_path = None
        try:
            user_file_path = self._get_file_path(user_file)
        except:
            error_msg = ("SANSCopyUserFileDependency: There was an issue "
                         "finding the specfied user file. Make sure the "
                         "user file can be found")
            raise RuntimeError(error_msg)
        return user_file_path

    def _check_if_directory_exists(self, directory):
        '''
        Checks if the directory exists
        @param directory: the output directory
        @raises RuntimeError: if the output directory does not exist
        '''
        if not os.path.isdir(directory):
            error_msg = ("SANSCopyUserFileDependency: Could not find the "
                         "output directory.")
            raise RuntimeError(error_msg)

    def _get_file_path(self, incomplete_path):
        this_path = FileFinder.getFullPath(incomplete_path)
        if not this_path:
            # do not catch exception at this point
            this_path = FileFinder.findRuns(incomplete_path)
            # if list, get first value
            if hasattr(this_path, '__iter__'):
                this_path = this_path[0]
        return this_path

    def _remove_copied_files(self, copied_files):
        '''
        We attempt to remove all copied files. This clean up
        opeation could occur in a except block, hence we don't want any
        exception to pass out of this method.
        @param copied_files: a zip of copied files and targets
        '''
        try:
            for copied_file in copied_files:
                os.remove(copied_file)
        except OSError:
            pass

#pylint: disable=too-few-public-methods
class UserFileDependencyExtractor(object):
    '''
    Extracts all depenency file names from a user file
    '''
    def __init__(self):
        super(UserFileDependencyExtractor, self).__init__()
        file_types = [".nxs", ".xml", ".txt"]
        regexes_front = [r"=[\s]*[\w]+", r",[\s]*[\w]+"]
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
