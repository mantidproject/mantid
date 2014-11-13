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
    # Import MantidPlot python commands
    import mantidplot
    from mantidplot import *

    # Make Mantid available
    import mantid
    # Make everything available without imports
    from mantid import *
    from mantid.kernel import *
    from mantid.geometry import *
    from mantid.api import *
    from mantid.simpleapi import *

    # Define a helper class for the autocomplete
    import inspect
    import __main__

    def _ScopeInspector_GetFunctionAttributes(definitions):
        if type(definitions) != dict:
            return []
        from mantid.simpleapi import _get_function_spec
        keywords = []
        for name,obj in definitions.iteritems():
            if name.startswith('_') : continue
            if inspect.isclass(obj) or inspect.ismodule(obj):
                continue
            if inspect.isfunction(obj) or inspect.isbuiltin(obj):
                keywords.append(name + _get_function_spec(obj))
                continue
            # Object could be a proxy so check and use underlying object
            if hasattr(obj,"_getHeldObject"):
                obj = obj._getHeldObject()
            attrs = dir(obj)
            for att in attrs:
                try:
                    fattr = getattr(obj,att)
                except Exception:
                    continue # not much we do if not even calling it causes an exception
                if att.startswith('_'):
                    continue
                if inspect.isfunction(fattr) or inspect.ismethod(fattr) or \
                        hasattr(fattr,'im_func'):
                    keywords.append(name + '.' + att + _get_function_spec(fattr))

        return keywords;

    import sys
    sys.path.insert(0,'')


else:
    raise ImportError("mantidplotrc.py is an initialization file for MantidPlot not an importable module")
