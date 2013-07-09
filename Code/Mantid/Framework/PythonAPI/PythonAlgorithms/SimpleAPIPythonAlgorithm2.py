
from mantid.api import PythonAlgorithm, AlgorithmFactory
import mantid.simpleapi as api
from mantid.simpleapi import *

class SimpleAPIPythonAlgorithm2(PythonAlgorithm):

    def PyInit(self):
        pass
    def PyExec(self):
        pass
        pass
        
AlgorithmFactory.subscribe(SimpleAPIPythonAlgorithm2)
