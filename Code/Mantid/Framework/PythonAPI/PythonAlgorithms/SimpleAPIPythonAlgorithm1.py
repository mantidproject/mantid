
from mantid.api import PythonAlgorithm, AlgorithmFactory
import mantid.simpleapi as api
from mantid.simpleapi import *

class SimpleAPIPythonAlgorithm1(PythonAlgorithm):

    def PyInit(self):
        pass
    def PyExec(self):
        SimpleAPIPythonAlgorithm2()
        api.SimpleAPIPythonAlgorithm2()
        
AlgorithmFactory.subscribe(SimpleAPIPythonAlgorithm1)
