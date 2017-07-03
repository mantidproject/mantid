# pylint: disable=too-few-public-methods,redefined-builtin
from __future__ import (absolute_import, division, print_function)
from six import iteritems

from mantid.api import Algorithm


class VesuvioBase(Algorithm):

    # There seems to be a problem with Python algorithms
    # defining a __init__ method
    _INST = None

    def _execute_child_alg(self, name, **kwargs):
        """Execute an algorithms as a child.
        By default all outputs are returned but this can be limited
        by providing the return_values=[] keyword
        """
        alg = self.createChildAlgorithm(name)
        # For Fit algorithm, Function & InputWorkspace have to
        # be set first and in that order.
        if name == 'Fit':
            for key in ('Function', 'InputWorkspace'):
                alg.setProperty(key, kwargs[key])
                del kwargs[key]

        ret_props = None
        if 'return_values' in kwargs:
            ret_props = kwargs['return_values']
            if type(ret_props) is str:
                ret_props = [ret_props]
            del kwargs['return_values']

        for name, value in iteritems(kwargs):
            alg.setProperty(name, value)
        alg.execute()

        # Assemble return values
        if ret_props is None:
            # This must be AFTER execute just in case that attached more
            # output properties
            ret_props = alg.outputProperties()
        outputs = []
        for name in ret_props:
            outputs.append(alg.getProperty(name).value)
        if len(outputs) == 1:
            return outputs[0]
        else:
            return tuple(outputs)

# -----------------------------------------------------------------------------------------
# Helper to translate from an table workspace to a dictionary. Should be on the workspace
# really ...
# -----------------------------------------------------------------------------------------


class TableWorkspaceDictionaryFacade(object):
    """
    Allows an underlying table workspace to be treated like a read-only dictionary
    """

    def __init__(self, held_object):
        self._table_ws = held_object

    def __getitem__(self, item):
        for row in self._table_ws:
            if row['Name'] == item:
                return row['Value']
        #endfor
        raise KeyError(str(item))

    def __contains__(self, item):
        for row in self._table_ws:
            if row['Name'] == item:
                return True

        return False

# -----------------------------------------------------------------------------------------
