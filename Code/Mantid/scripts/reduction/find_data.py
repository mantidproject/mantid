import os
from MantidFramework import *
from mantidsimple import *

def find_file(filename=None, startswith=None, data_dir=None):
    """
        Returns a list of file paths for the search criteria.
        @param filename: exact name of a file. The first file found will be returned.
        @param startswith: string that files should start with.
        @param data_dir: additional directory to search
    """
    # Files found
    files_found = []
    
    # List of directory to look into
    # The preferred way to operate is to have a symbolic link in a work directory,
    # so look in the current working directory first
    search_dirs = [os.getcwd()]
    # The second best location would be with the data itself
    if data_dir is not None:
        search_dirs.append(data_dir)
    # The standard place would be the location of the configuration files on the SNS mount
    search_dirs.append("/SNS/EQSANS/shared/instrument_configuration/")
    search_dirs.extend(ConfigService().getDataSearchDirs())
    
    # Look for specific file
    if filename is not None:
        for d in search_dirs:
            if not os.path.isdir(d):
                continue
            file_path = os.path.join(os.path.normcase(d), filename)
            if os.path.isfile(file_path):
                files_found.append(file_path)
                # If we are looking for a specific file, return right after we find the first
                if startswith is None:
                    return files_found

    # Look for files that starts with a specific string
    if startswith is not None:
        for d in search_dirs:
            if not os.path.isdir(d):
                continue
            files = os.listdir(d)
            for file in files:
                if file.startswith(startswith):
                    file_path = os.path.join(os.path.normcase(d), file)
                    files_found.append(file_path)

    return files_found

def find_data(file, data_dir=None, run_to_file_func=None):
    """
        Finds a file path for the specified data set, which can either be:
            - a run number
            - an absolute path
            - a file name
        @param file: file name or part of a file name
        @param data_dir: additional data directory to look into
        @param run_to_file_func: function that generates a file name given a run number
    """
    # First, check whether it's an absolute path
    if os.path.isfile(file):
        return file
    
    # Second, check whether it's a file name. If so, return the best match.
    files_found = find_file(filename=file, data_dir=data_dir)
    if len(files_found)>0:
        return files_found[0]
    
    # Third, build a file name assuming it's a run number
    if run_to_file_func is not None:
        filename = run_to_file_func(file)
        if filename is not None:
            files_found = find_file(filename=filename, data_dir=data_dir)
            if len(files_found)>0:
                return files_found[0]
    
    # Fourth, stay compatible with ISIS
    system_path = MantidFramework.FileFinder.getFullPath(file).strip()
    if system_path :
        return system_path

    #TODO: Finally, look in the catalog...
    
    # If we didn't find anything, raise an exception
    raise RuntimeError, "Could not find a file for [%s]" % str(file)
