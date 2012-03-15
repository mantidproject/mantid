"""
Defines functions to dynamically load Python modules.

These modules may define extensions to C++ types, e.g. 
algorithms, fit functions etc. 
"""
import os as _os
import imp as _imp
from mantid.kernel import Logger, ConfigService


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
            Load the module we are pointing at and return
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
    loaded = []
    if _os.path.isfile(path) and path.endswith('.py'): # Single file
        loader = PluginLoader(path)
        module = loader.run()
        loaded.append(module.__name__)
    elif _os.path.isdir(path): # Directory
        loaded.extend(load_from_dir(path))
    else: # a list 
        if ';' in path:
            path = split(';')
        if type(path) is list: # Call load again for each one
            for p in path: 
                loaded.extend(load(p))

    return loaded
    
def load_from_dir(directory):
    """
        Load all modules in the given directory
        
        @param directory :: A path that must point to a directory
    """
    if not _os.path.isdir(directory):
        raise RuntimeError("The path given does not point to an existing directory")
    loaded = []
    for root, dirs, files in _os.walk(directory):
        for f in files:
            filename = _os.path.join(root, f)
            loaded.extend(load(filename))
            
    return loaded
    
###############################################################################
# Backwards compatible loader for PythonAlgorithms written in old style
# 
# This will be removed when the old-style API is removed.
#
###############################################################################
class PyAlgLoader(object):

    __CHECKLINES__ = 100
    
    def __init__(self):
        self._logger = Logger.get("PyAlgLoader")

    def load_modules(self, refresh=False):
        """
        Import Python modules containing Python algorithms
        """
        dir_list = ConfigService["pythonalgorithms.directories"].split(';')
       
        # Check defined Python algorithm directories and load any modules
        changes = False
        for path in dir_list:
            if path == '':
                continue
            if self._importAlgorithms(path, refresh):
                changes = True

#
# ------- Private methods --------------
#
    def _importAlgorithms(self, path, refresh):
        # Make sure the directory doesn't contain a trailing slash
        path = path.rstrip("/").rstrip("\\")
        try:
            files = _os.listdir(path)
        except(OSError):
            return False
        changes = False
        
        def _process_file(file_path, modname):
            import sys
            pyext = '.py'
            if not modname.endswith(pyext):
                return
            original = _os.path.join(file_path, modname)
            modname = modname[:-len(pyext)]
            compiled = _os.path.join(file_path, modname + '.pyc')
            if modname in sys.modules and \
               _os.path.exists(compiled) and \
               _os.path.getmtime(compiled) >= _os.path.getmtime(original):
                return
            try:               
                if self._containsPyAlgorithm(original):
                    # Temporarily insert into path
                    sys.path.insert(0, file_path)
                    if modname in sys.modules:
                        reload(sys.modules[modname])
                    else:
                        __import__(modname)
                    changes = True
                    # Cleanup system path
                    del sys.path[0]
            except(StandardError), exp:
                self._logger.warning('Error: Importing module "%s" failed". %s' % (modname,str(exp)))
            except:
                self._logger.warning('Error: Unknown error on Python algorithm module import. "%s" skipped' % modname)

        # Find sub-directories     
        for root, dirs, files in _os.walk(path):
            for f in files:
                _process_file(root, f)
            
        return changes

    def _containsPyAlgorithm(self, modfilename):
        file = open(modfilename,'r')
        line_count = 0
        alg_found = False
        while line_count < self.__CHECKLINES__:
            line = file.readline()
            # EOF
            if line == '':
                alg_found = False
                break
            if line.rfind('PythonAlgorithm') >= 0:
                alg_found = True
                break
            line_count += 1
        file.close()
        return alg_found
   