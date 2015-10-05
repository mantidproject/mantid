#pylint: disable=invalid-name
"""
    Command set for Direct Geometry reduction
"""
# Import the specific commands that we need
from mantid.api import AlgorithmManager
from reduction.command_interface import *
from inelastic_reducer import InelasticReducer

def ARCS():
    Clear(InelasticReducer)

def CNCS():
    Clear(InelasticReducer)

def HYSPEC():
    Clear(InelasticReducer)

def LET():
    Clear(InelasticReducer)

def MAPS():
    Clear(InelasticReducer)

def MARI():
    Clear(InelasticReducer)

def MERLIN():
    Clear(InelasticReducer)

def SEQUOIA():
    Clear(InelasticReducer)

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
