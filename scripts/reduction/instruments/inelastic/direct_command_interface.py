# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Command set for Direct Geometry reduction
"""

# Import the specific commands that we need
from mantid.api import AlgorithmManager
from reduction.command_interface import Clear, ReductionSingleton
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


def FixEi(ei):
    alg = AlgorithmManager.create("InelasticFixEi")
    alg.setProperty("Ei", ei)
    ReductionSingleton().set_ei_calculator(alg)


def CalculateEi(guess=None):
    alg = AlgorithmManager.create("InelasticCalcEi")
    alg.setProperty("EiGuess", guess)
    ReductionSingleton().set_ei_calculator(alg)


def GetEiFromLog():
    print("GetEiFromLog(): *** NOT IMPLEMENTED *** ")
