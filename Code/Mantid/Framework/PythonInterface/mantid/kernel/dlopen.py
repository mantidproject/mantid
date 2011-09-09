"""
Provides a function to load a module that is a dynamic library such that the
flags used to open the shared library ensure that all symbols are imported
by default.

For Mantid this ensures the singleton symbols are wired up correctly
"""
import sys
import os
import platform

#######################################################################
# Ensure the correct Mantid shared libaries are found when loading the
# python shared libraries
#######################################################################
if sys.platform.startswith('win32'):
    _var = 'PATH'
    _sep = ';'
else:
    _sep = ':'
    if sys.platform.startswith('linux'):
        _var = 'LD_LIBRARY_PATH'
    elif sys.platform.startswith('darwin'):
        _var = 'DYLD_LIBRARY_PATH'
    else: # Give it a go how it is
        _var = ''
        _sep = ''
_oldpath = os.environ[_var]
os.environ[_var] = os.environ['MANTIDPATH'] + _sep + _oldpath       

#######################################################################
# Public api
#######################################################################
        
def setup_dlopen():
    """Set the flags for a call to import a shared library
    such that all symbols are imported.
    
    On Linux this sets the flags for dlopen so that 
    all symbols from the library are imported in to
    the global symbol table.
    
    Without this each shared library gets its own
    copy of any singleton, which is not the correct
    behaviour
    
    Returns the original flags
    """
    if platform.system() != "Linux": return None
    old_flags = sys.getdlopenflags()
    try:
        import DLFCN as dynload
    except:
        # Try older module
        try:
            import dl as dynload
        except:
            # If neither is available then this platform is unsupported
            print "Both the DLFCN and dl modules are unavailable."
            print "Cannot run Mantid from stand-alone Python on this platform."
            sys.exit(1)
    
    sys.setdlopenflags(dynload.RTLD_NOW | dynload.RTLD_GLOBAL)
    return old_flags
    
def restore_flags(flags):
    """Restores the dlopen flags to those provided,
    usually with the results from a call to setup_dlopen
    """
    if flags is None: return
    sys.setdlopenflags(flags)
