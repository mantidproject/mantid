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
if __name__ == '__main__':
    
    # Make Mantid available
    from MantidFramework import *
    # Initialize the Mantid framework
    mtd.initialise()

    # Make MantidPlot Python API available to main user scripts.
    # For modules imported into a main script users will need to do this too
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
                keywords.append(name + ScopeInspector_GetFunctionSpec(obj))
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
                    keywords.append(name + '.' + att + ScopeInspector_GetFunctionSpec(fattr))

        return keywords;

    def ScopeInspector_GetFunctionSpec(func):
        try:
            argspec = inspect.getargspec(func)
        except TypeError:
            return ' '
        args = argspec[0]
        # For methods strip the self argument
        if hasattr(func, 'im_func'):
            args = args[1:]
        defs = argspec[3]
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
