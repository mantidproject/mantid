# pylint: disable=too-few-public-methods,redefined-builtin
from __future__ import (absolute_import, division, print_function)
from six import iteritems

from mantid.api import Algorithm

class VesuvioBase(Algorithm):

    # There seems to be a problem with Python algorithms
    # defining a __init__ method
    _INST = None

    # ----------------------------------------------------------------------------------------

    def _execute_child_alg(self, name, **kwargs):
        alg = self.createChildAlgorithm(name)
        # Function needs to be set before input ws for fit algs
        function_key = 'Function'
        if function_key in kwargs:
            alg.setProperty(function_key, kwargs[function_key])
            del kwargs[function_key]

        for name, value in iteritems(kwargs):
            alg.setProperty(name, value)
        alg.execute()
        outputs = list()
        for name in alg.outputProperties():
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
