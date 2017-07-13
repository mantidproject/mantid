from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *

class NewMCAbsorption(DataProcessorAlgorithm):

    # basic sample variables
    _input_ws_name = None
    _sample_chemical_formula = None
    _sample_density_type = None
    _sample_density = None
    _sample_height = None

    # general variables
    _events = None
    _interpolation = None
    _output_ws = None

    # beam variables
    _beam_height = None
    _beam_width = None

    # annulus variables
    _sample_inner_radius = None
    _sample_outer_radius = None

    # cylinder variables
    _sample_radius = None

    # flat plate variables
    _sample_width = None
    _sample_thickness = None
    _sample_angle = None


    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates absorption corrections for a given sample shape."


# Register algorithm with Mantid
AlgorithmFactory.subscribe(NewMCAbsorption)