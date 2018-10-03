# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Defines functions to dynamically load Python modules.

These modules may define extensions to C++ types, e.g.
algorithms, fit functions etc.
"""
from __future__ import (absolute_import, division,
                        print_function)

import os as _os
import sys as _sys
try:
    from importlib.machinery import SourceFileLoader
except ImportError:
    # imp is deprecated in Python 3 but importlib doesn't exist
    # in Python 2.
    # We only use a single function so implement a handwritten compatability
    # class
    import imp as _imp
    class SourceFileLoader(object):

        def __init__(self, name, pathname):
            self._name = name
            self._pathname = pathname

        def load_module(self):
            return _imp.load_source(self._name, self._pathname)
    #endclass

from . import logger, Logger, config

# String that separates paths (should be in the ConfigService)
PATH_SEPARATOR=";"

class PluginLoader(object):

    extension = ".py"

    def __init__(self, filepath):
        if not _os.path.isfile(filepath):
            raise ValueError("PluginLoader expects a single filename. '%s' does not point to an existing file" % filepath)
        if not filepath.endswith(self.extension):
            raise ValueError("PluginLoader expects a filename ending with .py. '%s' does not have a .py extension" % filepath)
        self._filepath = filepath
        self._logger = Logger("PluginLoader")

    def run(self):
        """
            Try and load the module we are pointing at and return
            the module object.

            Any ImportErrors raised are not caught and are passed
            on to the caller
        """
        pathname = self._filepath
        name = _os.path.basename(pathname) # Including extension
        name = _os.path.splitext(name)[0]
        self._logger.debug("Loading python plugin %s" % pathname)
        return SourceFileLoader(name, pathname).load_module()

#======================================================================================================================
# High-level functions to assist with loading
#======================================================================================================================

def get_plugin_paths_as_set(key):
    """
        Returns the value of the given key in the config service
        as a set. Raises an KeyError if the key is not defined

        @param key The name of the key
        @returns A set containing defined plugins paths
    """
    s = set(config[key].split(PATH_SEPARATOR))
    if '' in s:
        s.remove('')
    return s

def check_for_plugins(top_dir):
    """
        Runs a quick check to see if any plugin files exist in the given directory

        @returns True if any plugins are found, false otherwise
    """
    if not _os.path.isdir(top_dir):
        return False

    for root, dirs, files in _os.walk(top_dir):
        for f in files:
            if f.endswith(PluginLoader.extension):
                return True

    return False


def find_plugins(top_dir):
    """
       Searches recursively from the given directory to find the list of plugins that should be loaded
       @param top_dir :: A string containing a path to a directory. Throws ValueError if it is not valid
    """
    if not _os.path.isdir(top_dir):
        raise ValueError("Cannot search given path for plugins, path is not a directory: '%s' " % str(top_dir))
    all_plugins = []
    algs = []
    for root, dirs, files in _os.walk(top_dir):
        for f in files:
            if f.endswith(PluginLoader.extension):
                filename = _os.path.join(root, f)
                all_plugins.append(filename)
                if contains_algorithm(filename):
                    algs.append(filename)

    return all_plugins, algs

#======================================================================================================================

def load(path):
    """
        High-level function to import the module(s) on the given path.
        The module is imported using __import__ so any code not defined
        inside an if __name__ == '__main__' block is executed.

        @param path :: If the path is a filename load the file; if the
        path points to a directory load all files in the directory
        recursively; if the path contains a list of directories then
        all files in each are loaded in turn

        @return A list of the loaded modules. Note this
        will not included modules that will have attempted to be
        reloaded but had not been changed
    """
    if PATH_SEPARATOR in path:
        path = path.split(PATH_SEPARATOR)

    loaded = []
    if type(path) == list:
        loaded += load_from_list(path)
    elif _os.path.isfile(path) and path.endswith(PluginLoader.extension): # Single file
        loaded += load_from_file(path)
    elif _os.path.isdir(path):
        loaded += load_from_dir(path)
    else:
        raise RuntimeError("Unknown type of path found when trying to load plugins: '%s'" % str(path))

    return loaded

#======================================================================================================================

def load_from_list(paths):
    """
        Load all modules in the given list

        @param paths :: A list of filenames to load
    """
    loaded = []
    for p in paths:
        try:
            loaded += load(p)
        except RuntimeError:
            continue

    return loaded

#======================================================================================================================

def load_from_dir(directory):
    """
        Load all modules in the given directory

        @param directory :: A path that must point to a directory
    """
    plugins = find_plugins(directory)
    loaded = []
    for filepath in plugins:
        loaded += load(filepath)

    return loaded

#======================================================================================================================

def load_from_file(filepath):
    """
        Loads the plugin file. Any code present at the top-level will
        be executed on loading
        @param filepath :: A path that must point to a file
    """
    loaded = []
    try:
        name, module = load_plugin(filepath)
        loaded.append(module)
    except Exception as exc:
        logger.warning("Failed to load plugin %s. Error: %s" % (filepath, str(exc)))

    return loaded

#======================================================================================================================

def load_plugin(plugin_path):
    """
        Load a plugin and return the name & module object

         @param plugin_path :: A path that must should point
         to a .py file that will be loaded. A ValueError is raised if
         path is not a valid plugin path. Any exceptions raised by the
         import are passed to the caller
    """
    loader = PluginLoader(plugin_path)
    module = loader.run()
    return module.__name__, module

#======================================================================================================================

def sync_attrs(source_module, attrs, clients):
    """
        Syncs the attribute definitions between the
        given list from the source module & list of client modules such
        that the function defintions point to the same
        one
        @param source_module :: The module containing the "correct"
                                definitions
        @param attrs :: The list of attributes to change in the client modules
        @param clients :: A list of modules whose attribute definitions
                          should be taken from source
    """
    for func_name in attrs:
        attr = getattr(source_module, func_name)
        for plugin in clients:
            if hasattr(plugin, func_name):
                setattr(plugin, func_name, attr)

#======================================================================================================================

def contains_algorithm(filename):
    """
        Inspects the file to look for an algorithm registration line
    """
    if _sys.version_info[0] < 3:
        def readlines_reversed(f):
            return reversed(f.readlines())
    else:
        def readlines_reversed(f):
            return reversed(list(f.readlines()))
    #endif
    alg_found = True
    try:
        from io import open
        with open(filename,'r', encoding='UTF-8') as plugin_file:
            for line in readlines_reversed(plugin_file):
                if 'AlgorithmFactory.subscribe' in line:
                    alg_found = True
                    break
    except Exception as exc:
        # something wrong with reading the file
        logger.warning("Error checking plugin content in '{0}'\n{1}".format(filename,str(exc)))
        alg_found = False

    return alg_found
