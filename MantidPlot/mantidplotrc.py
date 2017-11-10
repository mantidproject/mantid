#-------------------------------------------------------------------------------
# mantidplotrc.py
#
# Startup script for MantidPlot, executed once when the python environment
# is initialized. Any definitions added here will affect all Python scopes
# within the program.
#
#-------------------------------------------------------------------------------
# Do NOT add __future__ imports here as they will NOT be confined to this
# file.

if __name__ == '__main__':
    try:
        import matplotlib as _mpl
        _mpl.use("module://pymantidplot.mpl.backend_mtdqt4agg")
        del _mpl
    except ImportError:
        pass

    from six import iteritems as _iteritems

    # Import MantidPlot python commands
    import mantidplot
    from mantidplot import *
    try:
        # The MantidPlot namespace is not ready for the python3-style range function
        # so we ensure we revert back to the current built-in version
        del range
    except NameError:
        pass

    # Make Mantid available
    import mantid
    # Make everything available without imports
    from mantid import *
    from mantid.kernel import *
    from mantid.geometry import *
    from mantid.api import *
    from mantid.simpleapi import *

    # Common imports (here for backwards compatability)
    import os
    import sys

    #---------------------------------------------------------------------------
    # Autocomplete helper method
    #---------------------------------------------------------------------------
    import inspect as _inspect
    import __main__

    def _ScopeInspector_GetFunctionAttributes(definitions):
        if type(definitions) != dict:
            return []
        from mantid.simpleapi import _get_function_spec
        keywords = []
        for name,obj in _iteritems(definitions):
            if name.startswith('_') : continue
            if _inspect.isclass(obj) or _inspect.ismodule(obj):
                continue
            if _inspect.isfunction(obj) or _inspect.isbuiltin(obj):
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
                if _inspect.isfunction(fattr) or _inspect.ismethod(fattr) or \
                        hasattr(fattr,'im_func'):
                    keywords.append(name + '.' + att + _get_function_spec(fattr))

        return keywords;
else:
    raise ImportError("mantidplotrc.py is an initialization file for MantidPlot not an importable module")
