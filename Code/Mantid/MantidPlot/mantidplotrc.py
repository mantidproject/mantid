#------------------------------------------------
# mantidplotrc.py
#
# Load Mantid Python API into MantidPlot by default.
#
# Note that this file is designed to be executed 
# by Python, i.e. using execfile and not imported
#
# Author: Martyn Gigg, Tessella Support Services plc
#
#----------------------------------------------
def get_default_python_api():
    """Returns an integer indicating which Python API to use

    There are currently two versions:
        1 - The original Python API
        2 - The new-style Python interface returning workspaces from simple algorithm functions
    """
    from PyQt4 import QtCore
    settings = QtCore.QSettings()
    settings.beginGroup("Mantid")
    settings.beginGroup("Python")
    api_version = settings.value("APIVersion", 2).toInt()
    settings.endGroup()
    settings.endGroup()
    if type(api_version) == tuple:
        return api_version[0]
    else:
        return api_version

if __name__ == '__main__':
    # Make Mantid available
    from MantidFramework import *
    # Initialize the Mantid framework
    mtd.initialise()
    # For some reason the algorithm definitions are not available within IPython
    # Adding this fixes that and appears to do no harm elsewhere
    from mantidsimple import *
    # Make Mantid available without requiring users to import scripts
    _api = get_default_python_api()
    if _api == 2:
        # Put the new one on top
        from mantid import *
        from mantid.simpleapi import *
    elif _api != 1:
        raise RuntimeError("Unknown Python API version requested: %d" % _api)
    else:
        pass

    # Import MantidPlot python commands
    from mantidplot import *
    
    # Define a helper class for the autocomplete
    import inspect
    import __main__

    def _ScopeInspector_GetFunctionAttributes(definitions):
        if type(definitions) != dict:
            return []
        keywords = []
        for name,obj in definitions.iteritems():
            if name.startswith('_') : continue
            if inspect.isclass(obj) or inspect.ismodule(obj):
                continue
            if inspect.isfunction(obj) or inspect.isbuiltin(obj):
                keywords.append(name + _ScopeInspector_GetFunctionSpec(obj))
                continue
            # Object could be a proxy so check and use underlying object
            if hasattr(obj,"_getHeldObject"):
                obj = obj._getHeldObject()
            attrs = dir(obj)
            for att in attrs:
                fattr = getattr(obj,att)
                if att.startswith('_'): 
                    continue
                if inspect.isfunction(fattr) or inspect.ismethod(fattr) or \
                        hasattr(fattr,'im_func'):
                    keywords.append(name + '.' + att + _ScopeInspector_GetFunctionSpec(fattr))

        return keywords;

    def _ScopeInspector_GetFunctionSpec(func):
        try:
            argspec = inspect.getargspec(func)
        except TypeError:
            return ' '
        # Algorithm functions have varargs set not args
        args = argspec[0]
        if args != []:
            # For methods strip the self argument
            if hasattr(func, 'im_func'):
                args = args[1:]
            defs = argspec[3]
        elif argspec[1] is not None:
            # Get from varargs/keywords
            arg_str = argspec[1].strip().lstrip('\b')
            defs = []
            # Keyword args
            kwargs = argspec[2]
            if kwargs is not None:
                kwargs = kwargs.strip().lstrip('\b\b')
                if kwargs == 'kwargs':
                    kwargs = '**' + kwargs + '=None'
                arg_str += ',%s' % kwargs
            # Any default argument appears in the string
            # on the rhs of an equal
            for arg in arg_str.split(','):
                arg = arg.strip()
                if '=' in arg:
                    arg_token = arg.split('=')
                    args.append(arg_token[0])
                    defs.append(arg_token[1])
                else:
                    args.append(arg)
            if len(defs) == 0: defs = None
        else:
            return ' '

        if defs is None:
            calltip = ','.join(args)
            calltip = '(' + calltip + ')'
        else:
            # The defaults list contains the default values for the last n arguments
            diff = len(args) - len(defs)
            calltip = ''
            for index in range(len(args) - 1, -1,-1):
                def_index = index - diff
                if def_index >= 0:
                    calltip = '[' + args[index] + '],' + calltip
                else:
                    calltip = args[index] + "," + calltip
            calltip = '(' + calltip.rstrip(',') + ')'
        return calltip
    
    import sys
    sys.path.insert(0,'')

    
else:
    raise ImportError("mantidplotrc.py is an initialization file for MantidPlot not an importable module")
