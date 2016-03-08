# pylint: disable=too-few-public-methods
from mantid.api import Algorithm

class VesuvioBase(Algorithm):

    # There seems to be a problem with Python algorithms
    # defining a __init__ method
    _INST = None

    # ----------------------------------------------------------------------------------------

    def _execute_child_alg(self, name, **kwargs):
        alg = self.createChildAlgorithm(name)
        for name, value in kwargs.iteritems():
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
