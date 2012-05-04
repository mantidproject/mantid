"""
Defines functions to dynamically load Python modules.

These modules may define extensions to C++ types, e.g. 
algorithms, fit functions etc. 
"""
import os as _os
import imp as _imp
from mantid.kernel import logger, Logger, ConfigService


class PluginLoader(object):

    def __init__(self, filepath):
        if not _os.path.isfile(filepath):
            raise ValueError("PluginLoader expects a single filename. '%s' does not point to an existing file" % filepath)
        if not filepath.endswith('.py'):
            raise ValueError("PluginLoader expects a filename ending with .py. '%s' does not have a .py extension" % filepath)
        self._filepath = filepath
        self._logger = Logger.get("PluginLoader")

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
        return _imp.load_source(name, pathname)

def load(path):
    """
        High-level function to import the module(s) on the given path. 
        The module is imported using __import__ so any code not defined
        inside an if __name__ == '__main__' block is executed.

        @param path :: If the path is a filename load the file; if the
        path points to a directory load all files in the directory 
        recursively; if the path contains a list of directories then
        all files in each are loaded in turn
        
        @return A list of the names of the loaded modules. Note this
        will not included modules that will have attempted to be
        reloaded but had not been changed
    """
    loaded = {}
    if _os.path.isfile(path) and path.endswith('.py'): # Single file
        try:
            if contains_newapi_algorithm(path):
                name, module = load_plugin(path)
                loaded[name] = module
        except Exception, exc:
            logger.warning("Failed to load plugin %s. Error: %s" % (path, str(exc)))
    elif _os.path.isdir(path): # Directory
        loaded.update(load_from_dir(path))
    else: # a list 
        if ';' in path:
            path = path.split(';')
        if type(path) is list: # Call load again for each one
            for p in path: 
                loaded.update(load(p))

    return loaded
    
def load_from_dir(directory):
    """
        Load all modules in the given directory
        
        @param directory :: A path that must point to a directory
    """
    if not _os.path.isdir(directory):
        raise RuntimeError("The path given does not point to an existing directory")
    loaded = {}
    for root, dirs, files in _os.walk(directory):
        for f in files:
            filename = _os.path.join(root, f)
            loaded.update(load(filename))
            
    return loaded

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

def contains_newapi_algorithm(filename):
    """
        Inspects the given file to check whether
        it contains an algorithm written with this API.
        The check is simple. If either the import
        MantidFramework or mantidsimple are discovered then
        it will not be considered a new API algorithm
        
        @param filename :: A full file path pointing to a python file
        @returns True if a python algorithm written with the new API
        has been found.
    """
    file = open(filename,'r')
    alg_found = True
    for line in reversed(file.readlines()):
        if 'registerPyAlgorithm' in line:
            alg_found = False
            break
    file.close()
    return alg_found
