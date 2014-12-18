"""
    Command set for Direct Geometry reduction
"""
# Import the specific commands that we need
import mantid
from mantid.api import AlgorithmManager
from reduction.command_interface import *
from inelastic_reducer import InelasticReducer

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
    alg = AlgorithmManager.create("InelasticFixEi")
    alg.setProperty("Ei", ei)
    ReductionSingleton().set_ei_calculator(alg)

def CalculateEi(guess=None):
    alg = AlgorithmManager.create("InelasticCalcEi")
    alg.setProperty("EiGuess",guess)
    ReductionSingleton().set_ei_calculator(alg)

def GetEiFromLog():
    print "GetEiFromLog(): *** NOT IMPLEMENTED *** "
