# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,
                        WorkspaceGroupProperty, PropertyMode)
from mantid.kernel import (VisibleWhenProperty, PropertyCriterion,
                           StringListValidator, IntBoundedValidator, FloatBoundedValidator, Direction)
from mantid.simpleapi import *


class SimpleShapeDiscusInelastic(PythonAlgorithm):
    # basic sample variables
    _chemical_formula = None
    _mass_density = None
    _shape = None
    _height = None

    # MC variables
    _single_paths = None
    _multiple_paths = None
    _scatterings = None
    _output_ws = None

    # flat plate variables
    _width = None
    _thickness = None
    _angle = None

    # cylinder variables
    _radius = None

    # annulus variables
    _inner_radius = None
    _outer_radius = None

    def category(self):
        return "Workflow\\MIDAS"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('ReducedWorkspace', '',
                                                     direction=Direction.Input),
                             doc='Reduced Workspace')
        self.declareProperty(MatrixWorkspaceProperty('SqwWorkspace', '',
                                                     direction=Direction.Input),
                             doc='S(Q,w) Workspace')
        self.declareProperty(WorkspaceGroupProperty(name='OutputWorkspace',
                                                    defaultValue='MuscatResults',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='Name for results workspaces')

        self.declareProperty(name='SampleMassDensity', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample mass density. Default=1.0')

        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             doc='Sample Chemical formula')

        self.declareProperty(name='Height', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height of the sample environment (cm)')

        # set up shape options

        self.declareProperty(name='Shape', defaultValue='FlatPlate',
                             validator=StringListValidator(['FlatPlate', 'Cylinder', 'Annulus']),
                             doc='Geometry of sample environment. Options are: FlatPlate, Cylinder, Annulus')

        flat_plate_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'FlatPlate')
        cylinder_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'Cylinder')
        annulus_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'Annulus')

        # flat plate options

        self.declareProperty(name='Width', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the FlatPlate sample environment (cm)')
        self.setPropertySettings('Width', flat_plate_condition)

        self.declareProperty(name='Thickness', defaultValue=0.1,
                             validator=FloatBoundedValidator(0.0),
                             doc='Thickness of the FlatPlate sample environment (cm)')
        self.setPropertySettings('Thickness', flat_plate_condition)

        self.declareProperty(name='Angle', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Angle of the FlatPlate sample environment with respect to the beam (degrees)')
        self.setPropertySettings('Angle', flat_plate_condition)

        # cylinder options

        self.declareProperty(name='SampleRadius', defaultValue=0.5,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample radius (cm). Default=0.5')
        self.setPropertySettings('SampleRadius', cylinder_condition)

        # annulus options

        self.declareProperty(name='SampleInnerRadius', defaultValue=0.5,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample inner radius (cm). Default=0.5')
        self.setPropertySettings('SampleInnerRadius', annulus_condition)

        self.declareProperty(name='SampleOuterRadius', defaultValue=0.6,
                             validator=FloatBoundedValidator(0.0),
                             doc='Sample outer radius (cm). Default=0.6')
        self.setPropertySettings('SampleOuterRadius', annulus_condition)

        # MC options

        self.declareProperty(name='NeutronPathsSingle', defaultValue=1000,
                             validator=IntBoundedValidator(1),
                             doc='Number of paths for single scattering. Default=1000')
        self.declareProperty(name='NeutronPathsMultiple', defaultValue=1000,
                             validator=IntBoundedValidator(1),
                             doc='Number of paths for multiple scattering. Default=1000')
        self.declareProperty(name='NumberScatterings', defaultValue=1,
                             validator=IntBoundedValidator(1),
                             doc='Number of scatterings. Default=1')

    def PyExec(self):

        reduced_ws = self.getProperty('ReducedWorkspace').value
        sqw_ws = self.getProperty('SqwWorkspace').value

        if self._shape == 'FlatPlate':
            width = self.getProperty('Width').value
            thick = self.getProperty('Thickness').value
            angle = self.getProperty('Angle').value

            SetSample(reduced_ws,
                      Geometry={"Shape": 'FlatPlate', "Height": self._height,"Width": width, "Angle": angle,
                                "Center": [0.,0.,0.], "Thick": thick},
                      Material={"ChemicalFormula": self._chemical_formula, "MassDensity": self._mass_density})

        if self._shape == 'Cylinder':
            sample_radius = self.getProperty('SampleRadius').value

            SetSample(reduced_ws,
                      Geometry={"Shape": "Cylinder", "Height": self._height,"Radius": sample_radius, "Center": [0.,0.,0.]},
                      Material={"ChemicalFormula": self._chemical_formula, "MassDensity": self._mass_density})

        if self._shape == 'Annulus':
            sample_inner = self.getProperty('SampleInnerRadius').value
            sample_outer = self.getProperty('SampleOuterRadius').value

            SetSample(reduced_ws,
                      Geometry={"Shape": "HollowCylinder", "Height": self._height,
                                "InnerRadius": sample_inner, "OuterRadius": sample_outer, "Center": [0.,0.,0.]},
                      Material={"ChemicalFormula": self._chemical_formula, "MassDensity": self._mass_density})

        results_group_ws = DiscusMultipleScatteringCorrection(InputWorkspace=reduced_ws,
                                                              StructureFactorWorkspace=sqw_ws,
                                                              OutputWorkspace=self._output_ws,
                                                              NeutronPathsSingle=self._single_paths,
                                                              NeutronPathsMultiple=self._multiple_paths,
                                                              NumberScatterings=self._scatterings,
                                                              startProgress=0.,
                                                              endProgress=1.)

        self.setProperty('OutputWorkspace', results_group_ws)

    def _setup(self):

        self._chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._mass_density = self.getProperty('SampleMassDensity').value

        # shape options
        self._shape = self.getProperty('Shape').value

        self._height = self.getProperty('Height').value

        # flat plate
        self._width = self.getProperty('Width').value
        self._thickness = self.getProperty('Thickness').value
        self._angle = self.getProperty('Angle').value

        # cylinder
        self._radius = self.getProperty('SampleRadius').value

        # annulus
        self._inner_radius = self.getProperty('SampleInnerRadius').value
        self._outer_radius = self.getProperty('SampleOuterRadius').value

        # MC parameters
        self._single_paths = self.getProperty('NeutronPathsSingle').value
        self._multiple_paths = self.getProperty('NeutronPathsMultiple').value
        self._scatterings = self.getProperty('NumberScatterings').value

        # output
        self._output_ws = self.getPropertyValue('OutputWorkspace')

    def validateInputs(self):

        self._setup()
        issues = dict()

        if not self._mass_density:
            issues['SampleMassDensity'] = 'Please enter a non-zero number for sample mass density'

        if not self._chemical_formula:
            issues['SampleChemicalFormula'] = 'Please enter a chemical formula.'

        if not self._height:
            issues['Height'] = 'Please enter a non-zero number for height'

        if self._shape == 'FlatPlate':
            if not self._width:
                issues['Width'] = 'Please enter a non-zero number for width'
            if not self._thickness:
                issues['Thickness'] = 'Please enter a non-zero number for thickness'

        if self._shape == 'Cylinder':
            if not self._radius:
                issues['Radius'] = 'Please enter a non-zero number for radius'

        if self._shape == 'Annulus':
            if not self._inner_radius:
                issues['InnerRadius'] = 'Please enter a non-zero number for inner radius'
            if not self._outer_radius:
                issues['OuterRadius'] = 'Please enter a non-zero number for outer radius'

            # Geometry validation: outer radius > inner radius
            if not self._outer_radius > self._inner_radius:
                issues['OuterRadius'] = 'Must be greater than InnerRadius'

        if not self._single_paths:
            issues['NeutronPathsSingle'] = 'Please enter a non-zero number for neutron paths single'

        if not self._multiple_paths:
            issues['NeutronPathsMultiple'] = 'Please enter a non-zero number for neutron paths multiple'

        if not self._scatterings:
            issues['NumberScatterings'] = 'Please enter a non-zero number for neutron scatterings'

        return issues


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SimpleShapeDiscusInelastic)
