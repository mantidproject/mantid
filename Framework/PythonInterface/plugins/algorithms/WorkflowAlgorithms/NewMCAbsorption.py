from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *


class NewMCAbsorption(DataProcessorAlgorithm):
    # basic sample variables
    _input_ws_name = None
    _chemical_formula = None
    _density_type = None
    _density = None
    _height = None

    # general variables
    _events = None
    _interpolation = None
    _output_ws = None

    # beam variables
    _beam_height = None
    _beam_width = None

    # flat plate variables
    _width = None
    _thickness = None
    _center = None
    _angle = None

    # cylinder variables
    _radius = None

    # annulus variables
    _inner_radius = None
    _outer_radius = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def summary(self):
        return "Calculates absorption corrections for a given sample shape."

    def PyInit(self):
        # basic sample options

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '', direction=Direction.Input),
                             doc="Input workspace")

        self.declareProperty(name="ChemicalFormula", defaultValue='', validator=StringMandatoryValidator(),
                             doc="Chemical formula of sample")

        self.declareProperty(name='DensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Use of Mass density or Number density')

        self.declareProperty(name='Density', defaultValue=0.1,
                             doc='Mass density (g/cm^3) or Number density (atoms/Angstrom^3)')

        # -------------------------------------------------------------------------------------------

        # Monte Carlo options
        self.declareProperty(name='NumberOfWavelengthPoints', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')

        self.declareProperty(name='EventsPerPoint', defaultValue=1000,
                             validator=IntBoundedValidator(0),
                             doc='Number of neutron events')

        self.declareProperty(name='Interpolation', defaultValue='Linear',
                             validator=StringListValidator(['Linear', 'CSpline']),
                             doc='Type of interpolation')

        # -------------------------------------------------------------------------------------------

        # Beam size
        self.declareProperty(name='BeamHeight', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height of the beam (cm)')

        self.declareProperty(name='BeamWidth', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the beam (cm)')

        # -------------------------------------------------------------------------------------------


        # set up shape options

        self.declareProperty(name="Shape", defaultValue="FlatPlate",
                             validator=StringListValidator(["FlatPlate", "Cylinder", "Annulus"]),
                             doc="Geometry of sample environment. Options are: FlatPlate, Cylinder, Annulus")

        flatPlateCondition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "FlatPlate")
        cylinderCondition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Cylinder")
        annulusCondition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Annulus")

        # height is common to all options

        self.declareProperty(name="Height", defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc="Height of the sample environment")

        # flat plate options

        self.declareProperty(name='Width', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the FlatPlate sample environment')
        self.setPropertySettings("Width", flatPlateCondition)

        self.declareProperty(name='Thickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(),
                             doc='Thickness of the FlatPlate sample environment')
        self.setPropertySettings("Thickness", flatPlateCondition)

        self.declareProperty(name='Center', defaultValue=0.0,
                             doc='Center of the FlatPlate sample environment')
        self.setPropertySettings("Center", flatPlateCondition)

        self.declareProperty(name='Angle', defaultValue=0.0,
                             doc='Angle of the FlatPlate sample environment')
        self.setPropertySettings("Angle", flatPlateCondition)

        # cylinder options

        self.declareProperty(name='Radius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Radius of the Cylinder sample environment')
        self.setPropertySettings("Radius", cylinderCondition)

        # annulus options

        self.declareProperty(name='OuterRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Outer radius of the Annulus sample environment')
        self.setPropertySettings("OuterRadius", annulusCondition)

        self.declareProperty(name='InnerRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Inner radius of the Annulus sample environment')
        self.setPropertySettings("InnerRadius", annulusCondition)

        # -------------------------------------------------------------------------------------------

        # Output options
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output corrected workspace.')

    def PyExec(self):
        pass


# Register algorithm with Mantid
AlgorithmFactory.subscribe(NewMCAbsorption)
