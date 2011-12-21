from MantidFramework import *


"""
    Command set for Direct Geometry reduction
"""
# Import the specific commands that we need
from reduction.command_interface import *
from inelastic_reducer import InelasticReducer

from mantidsimple import *

def ARCS():
    Clear(InelasticReducer)
    
    pass

def CNCS():
    Clear(InelasticReducer)
    pass
    
def HYSPEC():
    Clear(InelasticReducer)
    pass    
    
def LET():
    Clear(InelasticReducer)
    pass    
    
def MAPS():
    Clear(InelasticReducer)
    pass
    
def MARI():
    Clear(InelasticReducer)
    pass

def MERLIN():
    Clear(InelasticReducer)
    pass
    
def SEQUOIA():
    Clear(InelasticReducer)
    pass

def DefaultLoader():
    step = InelasticLoader()
    step.initialize()
    ReductionSingleton().set_loader(step)

def FixEi(ei):
    alg = mtd._createAlgProxy("InelasticFixEi")
    alg.setPropertyValues(Ei=ei)
    ReductionSingleton().set_ei_calculator(alg)
    
def CalculateEi(guess=None):
    #ReductionSingleton().set_ei_calculator(InelasticCalcEi, EiGuess=guess)
    alg = mtd._createAlgProxy("InelasticCalcEi")
    alg.setPropertyValues(EiGuess=guess)
    ReductionSingleton().set_ei_calculator(alg)

def GetEiFromLog():
    #ReductionSingleton().set_ei_calculator(InelasticGetEiFromLog)
    #alg = mtd._createAlgProxy("InelasticGetEiFromLog")
    #alg.setPropertyValues(EiGuess=guess)
    #ReductionSingleton().set_ei_calculator(alg)
    print "GetEiFromLog(): *** NOT IMPLEMENTED *** "
