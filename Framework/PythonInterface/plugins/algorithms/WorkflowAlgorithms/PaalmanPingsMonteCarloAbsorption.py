# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, PropertyMode, WorkspaceGroupProperty, SpectraAxis,
                        WorkspaceGroup, WorkspaceProperty, Sample)
from mantid.kernel import (VisibleWhenProperty, PropertyCriterion, StringListValidator, IntBoundedValidator,
                           FloatBoundedValidator, Direction, LogicOperator, EnabledWhenProperty)
from mantid.simpleapi import (config, logger, mtd)
import math
import numpy as np
import os.path


class PaalmanPingsMonteCarloAbsorption(DataProcessorAlgorithm):
    # General variables
    _emode = None
    _efixed = None
    _general_kwargs = None
    _shape = None
    _height = None
    _isis_instrument = None

    # Sample variables
    _sample_angle = None
    _sample_center = None
    _sample_chemical_formula = None
    _sample_density = None
    _sample_density_type = None
    _sample_inner_radius = None
    _sample_outer_radius = None
    _sample_radius = None
    _sample_thickness = None
    _sample_unit = None
    _sample_width = None
    _sample_ws = None

    # Container variables
    _container_angle = None
    _container_center = None
    _container_chemical_formula = None
    _container_density = None
    _container_density_type = None
    _container_radius = None
    _container_inner_radius = None
    _container_outer_radius = None
    _container_thickness = None
    _container_width = None

    # Output workspaces
    _ass_ws = None
    _acc_ws = None
    _assc_ws = None
    _acsc_ws = None
    _output_ws = None

    def category(self):
        return "Workflow\\Inelastic;CorrectionFunctions\\AbsorptionCorrections;Workflow\\MIDAS"

    def seeAlso(self):
        return ["MonteCarloAbsorption", "ApplyPaalmanPingsCorrection"]

    def summary(self):
        return "Calculates absorption corrections in Paalman & Pings formalism for a given sample " \
               "and optionally its environment using a Monte Carlo simulation."

    def checkGroups(self):
        return False

    def PyInit(self):
        # Sample Input
        self.declareProperty(WorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Sample Workspace')

        # Monte Carlo Options
        self.declareProperty(name='NumberOfWavelengthPoints', defaultValue=10,
                             validator=IntBoundedValidator(1),
                             doc='Number of wavelengths for calculation')
        self.declareProperty(name='EventsPerPoint', defaultValue=1000,
                             validator=IntBoundedValidator(0),
                             doc='Number of neutron events')
        self.declareProperty(name='Interpolation', defaultValue='Linear',
                             validator=StringListValidator(
                                 ['Linear', 'CSpline']),
                             doc='Type of interpolation')
        self.declareProperty(name='MaxScatterPtAttempts', defaultValue=5000,
                             validator=IntBoundedValidator(0),
                             doc='Maximum number of tries made to generate a scattering point')
        self.declareProperty(name='SparseInstrument', defaultValue=False,
                             doc='Whether to spatially approximate the instrument for faster calculation.')
        self.declareProperty(name='NumberOfDetectorRows', defaultValue=3,
                             validator=IntBoundedValidator(lower=3),
                             doc='Number of detector rows in the detector grid of the sparse instrument.')
        self.declareProperty(name='NumberOfDetectorColumns', defaultValue=2,
                             validator=IntBoundedValidator(lower=2),
                             doc='Number of detector columns in the detector grid of the sparse instrument.')

        sparse_condition = EnabledWhenProperty('SparseInstrument', PropertyCriterion.IsNotDefault)
        self.setPropertySettings('NumberOfDetectorRows', sparse_condition)
        self.setPropertySettings('NumberOfDetectorColumns', sparse_condition)

        self.setPropertyGroup('SparseInstrument', 'Monte Carlo Options')
        self.setPropertyGroup('NumberOfDetectorRows', 'Monte Carlo Options')
        self.setPropertyGroup('NumberOfDetectorColumns', 'Monte Carlo Options')
        self.setPropertyGroup('NumberOfWavelengthPoints', 'Monte Carlo Options')
        self.setPropertyGroup('EventsPerPoint', 'Monte Carlo Options')
        self.setPropertyGroup('Interpolation', 'Monte Carlo Options')
        self.setPropertyGroup('MaxScatterPtAttempts', 'Monte Carlo Options')

        # Beam Options
        self.declareProperty(name='BeamHeight', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Height of the beam (cm)')
        self.declareProperty(name='BeamWidth', defaultValue=1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the beam (cm)')

        self.setPropertyGroup('BeamHeight', 'Beam Options')
        self.setPropertyGroup('BeamWidth', 'Beam Options')

        # Shape Options
        self.declareProperty(name='Shape', defaultValue='FlatPlate',
                             validator=StringListValidator(['FlatPlate', 'Cylinder', 'Annulus']),
                             doc='Geometric shape of the sample environment')

        flat_plate_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'FlatPlate')
        cylinder_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'Cylinder')
        annulus_condition = VisibleWhenProperty('Shape', PropertyCriterion.IsEqualTo, 'Annulus')

        # height is common to all, and should be the same for sample and container
        self.declareProperty('Height', defaultValue=0.0, validator=FloatBoundedValidator(0.0),
                             doc='Height of the sample environment (cm)')

        self.setPropertyGroup('Shape', 'Shape Options')
        self.setPropertyGroup('Height', 'Shape Options')

        # Sample Shape
        # Flat Plate
        self.declareProperty(name='SampleWidth', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Width of the sample environment (cm)')
        self.declareProperty(name='SampleThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Thickness of the sample environment (cm)')
        self.declareProperty(name='SampleCenter', defaultValue=0.0,
                             doc='Center of the sample environment')
        self.declareProperty(name='SampleAngle', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Angle of the sample environment with respect to the beam (degrees)')

        self.setPropertySettings('SampleWidth', flat_plate_condition)
        self.setPropertySettings('SampleThickness', flat_plate_condition)
        self.setPropertySettings('SampleCenter', flat_plate_condition)
        self.setPropertySettings('SampleAngle', flat_plate_condition)

        self.setPropertyGroup('SampleWidth', 'Sample Shape')
        self.setPropertyGroup('SampleThickness', 'Sample Shape')
        self.setPropertyGroup('SampleCenter', 'Sample Shape')
        self.setPropertyGroup('SampleAngle', 'Sample Shape')

        # Cylinder
        self.declareProperty(name='SampleRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Radius of the sample environment (cm)')

        self.setPropertySettings('SampleRadius', cylinder_condition)
        self.setPropertyGroup('SampleRadius', 'Sample Shape')

        # Annulus
        self.declareProperty(name='SampleInnerRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Inner radius of the sample environment (cm)')
        self.declareProperty(name='SampleOuterRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Outer radius of the sample environment (cm)')

        self.setPropertySettings('SampleInnerRadius', annulus_condition)
        self.setPropertySettings('SampleOuterRadius', annulus_condition)

        self.setPropertyGroup('SampleInnerRadius', 'Sample Shape')
        self.setPropertyGroup('SampleOuterRadius', 'Sample Shape')

        # Sample Material
        self.declareProperty(name='SampleMaterialAlreadyDefined', defaultValue=False,
                             doc='Select this option if sample material has already been defined')
        self.declareProperty(name='SampleChemicalFormula', defaultValue='',
                             doc='Chemical formula for the sample material')
        self.declareProperty(name='SampleCoherentXSection', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='The coherent cross-section for the sample material in barns. To be used instead of '
                                 'Chemical Formula.')
        self.declareProperty(name='SampleIncoherentXSection', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='The incoherent cross-section for the sample material in barns. To be used instead of '
                                 'Chemical Formula.')
        self.declareProperty(name='SampleAttenuationXSection', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='The absorption cross-section for the sample material in barns. To be used instead of '
                                 'Chemical Formula.')
        self.declareProperty(name='SampleDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Use of Mass density or Number density for the sample.')
        self.declareProperty(name='SampleNumberDensityUnit', defaultValue='Atoms',
                             validator=StringListValidator(['Atoms', 'Formula Units']),
                             doc='Choose which units SampleDensity refers to. Allowed values: [Atoms, Formula Units]')
        self.declareProperty(name='SampleDensity', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='The value for the sample Mass density (g/cm^3) or Number density (1/Angstrom^3).')

        self.setPropertyGroup('SampleMaterialAlreadyDefined', 'Sample Material')
        self.setPropertyGroup('SampleChemicalFormula', 'Sample Material')
        self.setPropertyGroup('SampleCoherentXSection', 'Sample Material')
        self.setPropertyGroup('SampleIncoherentXSection', 'Sample Material')
        self.setPropertyGroup('SampleAttenuationXSection', 'Sample Material')
        self.setPropertyGroup('SampleDensityType', 'Sample Material')
        self.setPropertyGroup('SampleNumberDensityUnit', 'Sample Material')
        self.setPropertyGroup('SampleDensity', 'Sample Material')

        # Container Input
        self.declareProperty(WorkspaceProperty('ContainerWorkspace', '', direction=Direction.Input,
                                               optional=PropertyMode.Optional),
                             doc='Container Workspace')

        container_condition = VisibleWhenProperty('ContainerWorkspace', PropertyCriterion.IsNotDefault)

        # Container Shape
        container_flat_plate_condition = VisibleWhenProperty(container_condition, flat_plate_condition,
                                                             LogicOperator.And)
        container_cylinder_condition = VisibleWhenProperty(container_condition, cylinder_condition,
                                                           LogicOperator.And)
        container_annulus_condition = VisibleWhenProperty(container_condition, annulus_condition,
                                                          LogicOperator.And)

        # Flat Plate
        self.declareProperty(name='ContainerFrontThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Front thickness of the container environment (cm)')
        self.declareProperty(name='ContainerBackThickness', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Back thickness of the container environment (cm)')

        self.setPropertySettings('ContainerFrontThickness', container_flat_plate_condition)
        self.setPropertySettings('ContainerBackThickness', container_flat_plate_condition)
        self.setPropertyGroup('ContainerFrontThickness', 'Container Shape')
        self.setPropertyGroup('ContainerBackThickness', 'Container Shape')

        # Cylinder
        self.declareProperty(name='ContainerRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Outer radius of the sample environment (cm)')

        self.setPropertySettings('ContainerRadius', container_cylinder_condition)
        self.setPropertyGroup('ContainerRadius', 'Container Shape')

        # Annulus
        self.declareProperty(name='ContainerInnerRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Inner radius of the container environment (cm)')
        self.declareProperty(name='ContainerOuterRadius', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='Outer radius of the container environment (cm)')

        self.setPropertySettings('ContainerInnerRadius', container_annulus_condition)
        self.setPropertySettings('ContainerOuterRadius', container_annulus_condition)

        self.setPropertyGroup('ContainerInnerRadius', 'Container Shape')
        self.setPropertyGroup('ContainerOuterRadius', 'Container Shape')

        # Container Material
        self.declareProperty(name='ContainerMaterialAlreadyDefined', defaultValue=False,
                             doc='Select this option if container material has already been defined')
        self.declareProperty(name='ContainerChemicalFormula', defaultValue='',
                             doc='Chemical formula for the container material')
        self.declareProperty(name='ContainerCoherentXSection', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='The coherent cross-section for the can material in barns. To be used instead of '
                                 'Chemical Formula.')
        self.declareProperty(name='ContainerIncoherentXSection', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='The incoherent cross-section for the can material in barns. To be used instead of '
                                 'Chemical Formula.')
        self.declareProperty(name='ContainerAttenuationXSection', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='The absorption cross-section for the can material in barns. To be used instead of '
                                 'Chemical Formula.')
        self.declareProperty(name='ContainerDensityType', defaultValue='Mass Density',
                             validator=StringListValidator(['Mass Density', 'Number Density']),
                             doc='Use of Mass density or Number density for the container.')
        self.declareProperty(name='ContainerNumberDensityUnit', defaultValue='Atoms',
                             validator=StringListValidator(['Atoms', 'Formula Units']),
                             doc='Choose which units ContainerDensity refers to. Allowed values: [Atoms, Formula Units]')
        self.declareProperty(name='ContainerDensity', defaultValue=0.0,
                             validator=FloatBoundedValidator(0.0),
                             doc='The value for the container Mass density (g/cm^3) or Number density (1/Angstrom^3).')

        self.setPropertyGroup('ContainerMaterialAlreadyDefined', 'Container Material')
        self.setPropertyGroup('ContainerChemicalFormula', 'Container Material')
        self.setPropertyGroup('ContainerCoherentXSection', 'Container Material')
        self.setPropertyGroup('ContainerIncoherentXSection', 'Container Material')
        self.setPropertyGroup('ContainerAttenuationXSection', 'Container Material')
        self.setPropertyGroup('ContainerDensityType', 'Container Material')
        self.setPropertyGroup('ContainerNumberDensityUnit', 'Container Material')
        self.setPropertyGroup('ContainerDensity', 'Container Material')

        self.setPropertySettings('ContainerMaterialAlreadyDefined', container_condition)
        self.setPropertySettings('ContainerChemicalFormula', container_condition)
        self.setPropertySettings('ContainerDensityType', container_condition)
        self.setPropertySettings('ContainerDensity', container_condition)

        # Output Workspace Group
        self.declareProperty(WorkspaceGroupProperty(name='CorrectionsWorkspace',
                                                    defaultValue='corrections',
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc='Name of the workspace group to save correction factors')

    def PyExec(self):

        progess_steps = 1. if not self._container_ws else 0.25

        sample_wave_ws = self._convert_to_wavelength(self._sample_ws)
        self._set_beam(sample_wave_ws)
        sample_wave_ws.setSample(Sample())
        self._set_sample(sample_wave_ws, ['Sample'])
        monte_carlo_alg = self.createChildAlgorithm("MonteCarloAbsorption", enableLogging=True,
                                                    startProgress=0, endProgress=progess_steps)
        self._set_algorithm_properties(monte_carlo_alg, self._monte_carlo_kwargs)
        monte_carlo_alg.setProperty("InputWorkspace", sample_wave_ws)
        monte_carlo_alg.setProperty("OutputWorkspace", self._ass_ws_name)
        monte_carlo_alg.setProperty("SimulateScatteringPointIn", "SampleOnly")
        monte_carlo_alg.execute()
        ass_ws = monte_carlo_alg.getProperty("OutputWorkspace").value
        ass_ws = self._convert_from_wavelength(ass_ws)
        mtd.addOrReplace(self._ass_ws_name, ass_ws)
        self._output_ws = self._group_ws([ass_ws])

        if self._container_ws:
            self._set_sample(sample_wave_ws, ['Sample', 'Container'])
            monte_carlo_alg_ssc = self.createChildAlgorithm("MonteCarloAbsorption", enableLogging=True,
                                                            startProgress=progess_steps, endProgress=2*progess_steps)
            self._set_algorithm_properties(monte_carlo_alg_ssc, self._monte_carlo_kwargs)
            monte_carlo_alg_ssc.setProperty("InputWorkspace", sample_wave_ws)
            monte_carlo_alg_ssc.setProperty("OutputWorkspace", self._assc_ws_name)
            monte_carlo_alg_ssc.setProperty("SimulateScatteringPointIn", "SampleOnly")
            monte_carlo_alg_ssc.execute()
            assc_ws = monte_carlo_alg_ssc.getProperty("OutputWorkspace").value
            assc_ws = self._convert_from_wavelength(assc_ws)
            mtd.addOrReplace(self._assc_ws_name, assc_ws)

            can_wave_ws = self._convert_to_wavelength(self._container_ws)
            self._set_beam(can_wave_ws)

            # since container can not exist without a valid sample, we work around this by setting container as sample
            self._set_sample(can_wave_ws, ['Container'], True)
            monte_carlo_alg_cc = self.createChildAlgorithm("MonteCarloAbsorption", enableLogging=True,
                                                           startProgress=2*progess_steps, endProgress=3*progess_steps)
            self._set_algorithm_properties(monte_carlo_alg_cc, self._monte_carlo_kwargs)
            monte_carlo_alg_cc.setProperty("InputWorkspace", can_wave_ws)
            monte_carlo_alg_cc.setProperty("OutputWorkspace", self._acc_ws_name)
            monte_carlo_alg_cc.setProperty("SimulateScatteringPointIn", "SampleOnly")
            monte_carlo_alg_cc.execute()
            acc_ws = monte_carlo_alg_cc.getProperty("OutputWorkspace").value
            acc_ws = self._convert_from_wavelength(acc_ws)
            mtd.addOrReplace(self._acc_ws_name, acc_ws)

            self._set_sample(can_wave_ws, ['Sample', 'Container'])
            monte_carlo_alg_csc = self.createChildAlgorithm("MonteCarloAbsorption", enableLogging=True,
                                                            startProgress=3*progess_steps, endProgress=1.)
            self._set_algorithm_properties(monte_carlo_alg_csc, self._monte_carlo_kwargs)
            monte_carlo_alg_csc.setProperty("InputWorkspace", can_wave_ws)
            monte_carlo_alg_csc.setProperty("OutputWorkspace", self._acsc_ws_name)
            monte_carlo_alg_csc.setProperty("SimulateScatteringPointIn", "EnvironmentOnly")
            monte_carlo_alg_csc.execute()
            acsc_ws = monte_carlo_alg_csc.getProperty("OutputWorkspace").value
            acsc_ws = self._convert_from_wavelength(acsc_ws)
            mtd.addOrReplace(self._acsc_ws_name, acsc_ws)

            self._output_ws = self._group_ws([ass_ws, assc_ws, acsc_ws, acc_ws])

        self.setProperty('CorrectionsWorkspace', self._output_ws)

    def _set_beam(self, ws):

        set_beam_alg = self.createChildAlgorithm("SetBeam", enableLogging=False)
        set_beam_alg.setProperty("InputWorkspace", ws)
        set_beam_alg.setProperty("Geometry", {'Shape': 'Slit',
                                              'Width': self._beam_width,
                                              'Height': self._beam_height})
        set_beam_alg.execute()

    def _set_sample(self, ws, components, can_as_sample=False):

        ws.setSample(Sample())
        set_sample_alg = self.createChildAlgorithm("SetSample", enableLogging=False)
        set_sample_alg.setProperty("InputWorkspace", ws)

        if 'Sample' in components:
            sample_geometry = dict()
            sample_geometry['Height'] = self._height

            if self._shape == 'FlatPlate':
                sample_geometry['Shape'] = 'FlatPlate'
                sample_geometry['Width'] = self._sample_width
                sample_geometry['Thick'] = self._sample_thickness
                sample_geometry['Center'] = [0.0, 0.0, self._sample_center]
                sample_geometry['Angle'] = self._sample_angle

            if self._shape == 'Cylinder':
                sample_geometry['Shape'] = 'Cylinder'
                sample_geometry['Radius'] = self._sample_radius
                sample_geometry['Center'] = [0.0, 0.0, 0.0]

            if self._shape == 'Annulus':
                sample_geometry['Shape'] = 'HollowCylinder'
                sample_geometry['InnerRadius'] = self._sample_inner_radius
                sample_geometry['OuterRadius'] = self._sample_outer_radius
                sample_geometry['Center'] = [0.0, 0.0, 0.0]

            set_sample_alg.setProperty("Geometry", sample_geometry)

            if not self._sample_material_defined:
                sample_material = dict()
                if self._set_sample_method == 'Chemical Formula':
                    sample_material['ChemicalFormula'] = self._sample_chemical_formula
                else:
                    sample_material['CoherentXSection'] = self._sample_coherent_cross_section
                    sample_material['IncoherentXSection'] = self._sample_incoherent_cross_section
                    sample_material['AttenuationXSection'] = self._sample_attenuation_cross_section
                    sample_material['ScatteringXSection'] = self._sample_coherent_cross_section + self._sample_incoherent_cross_section

                if self._sample_density_type == 'Mass Density':
                    sample_material['SampleMassDensity'] = self._sample_density
                if self._sample_density_type == 'Number Density':
                    sample_material['SampleNumberDensity'] = self._sample_density
                    sample_material['NumberDensityUnit'] = self._sample_number_density_unit
                set_sample_alg.setProperty("Material", sample_material)

        if 'Container' in components:
            container_geometry = dict()
            container_geometry['Height'] = self._height

            if self._shape == 'FlatPlate':
                container_geometry['Shape'] = 'FlatPlateHolder'
                container_geometry['Width'] = self._sample_width
                # we need to know the thickness and the centre of the sample as well,
                # in order to calculate the center offsets for the container panels
                container_geometry['Thick'] = self._sample_thickness
                container_geometry['Center'] = [0.0, 0.0, self._sample_center]
                container_geometry['Angle'] = self._sample_angle
                container_geometry['FrontThick'] = self._container_front_thickness
                container_geometry['BackThick'] = self._container_back_thickness

            if self._shape == 'Cylinder':
                container_geometry['Shape'] = 'HollowCylinder'
                container_geometry['InnerRadius'] = self._sample_radius
                container_geometry['OuterRadius'] = self._container_radius
                container_geometry['Center'] = [0.0, 0.0, 0.0]

            if self._shape == 'Annulus':
                container_geometry['Shape'] = 'HollowCylinderHolder'
                container_geometry['InnerRadius'] = self._container_inner_radius
                container_geometry['InnerOuterRadius'] = self._sample_inner_radius
                container_geometry['OuterInnerRadius'] = self._sample_outer_radius
                container_geometry['OuterRadius'] = self._container_outer_radius
                container_geometry['Center'] = [0.0, 0.0, 0.0]

            if can_as_sample:
                set_sample_alg.setProperty("Geometry", container_geometry)
            else:
                set_sample_alg.setProperty("ContainerGeometry", container_geometry)

            if not self._container_material_defined:
                container_material = dict()
                if self._set_can_method == 'Chemical Formula':
                    container_material['ChemicalFormula'] = self._container_chemical_formula
                else:
                    container_material['CoherentXSection'] = self._container_coherent_cross_section
                    container_material['IncoherentXSection'] = self._container_incoherent_cross_section
                    container_material['AttenuationXSection'] = self._container_attenuation_cross_section
                    container_material[
                        'ScatteringXSection'] = self._container_coherent_cross_section + self._container_incoherent_cross_section

                if self._container_density_type == 'Mass Density':
                    container_material['SampleMassDensity'] = self._container_density
                if self._container_density_type == 'Number Density':
                    container_material['SampleNumberDensity'] = self._container_density
                    container_material['NumberDensityUnit'] = self._container_number_density_unit

                if can_as_sample:
                    set_sample_alg.setProperty("Material", container_material)
                else:
                    set_sample_alg.setProperty("ContainerMaterial", container_material)

        set_sample_alg.execute()

    def _setup(self):

        self._sample_ws = self.getProperty("SampleWorkspace").value
        self._container_ws = self.getProperty("ContainerWorkspace").value
        sample_is_group = isinstance(self._sample_ws, WorkspaceGroup)
        container_is_group = isinstance(self._container_ws, WorkspaceGroup)

        # We cannot support WorkspaceGroups as inputs, since the output of the algorithm itself is a group
        # and it is currently not possible to override processGroups in python
        if sample_is_group or container_is_group:
            raise RuntimeError("WorkspaceGroup inputs are currently not supported. "
                               "Please select the workspace items themselves.")

        self._beam_height = self.getProperty('BeamHeight').value
        self._beam_width = self.getProperty('BeamWidth').value

        self._monte_carlo_kwargs = {'NumberOfWavelengthPoints': self.getProperty('NumberOfWavelengthPoints').value,
                                    'EventsPerPoint': self.getProperty('EventsPerPoint').value,
                                    'Interpolation': self.getProperty('Interpolation').value,
                                    'MaxScatterPtAttempts': self.getProperty('MaxScatterPtAttempts').value,
                                    'SparseInstrument': self.getProperty('SparseInstrument').value,
                                    'NumberOfDetectorRows': self.getProperty('NumberOfDetectorRows').value,
                                    'NumberOfDetectorColumns': self.getProperty('NumberOfDetectorColumns').value}

        self._shape = self.getProperty('Shape').value
        self._height = self.getProperty('Height').value

        self._sample_unit = self._sample_ws.getAxis(0).getUnit().unitID()
        if self._sample_unit == 'dSpacing':
            self._emode = 'Elastic'
        else:
            self._emode = str(self._sample_ws.getEMode())
        if self._emode == 'Indirect' or 'Direct':
            self._efixed = self._get_efixed()

        self._sample_material_defined = self.getProperty('SampleMaterialAlreadyDefined').value
        self._sample_chemical_formula = self.getPropertyValue('SampleChemicalFormula')
        self._sample_coherent_cross_section = self.getPropertyValue('SampleCoherentXSection')
        self._sample_incoherent_cross_section = self.getPropertyValue('SampleIncoherentXSection')
        self._sample_attenuation_cross_section = self.getPropertyValue('SampleAttenuationXSection')
        self._sample_density_type = self.getPropertyValue('SampleDensityType')
        self._sample_number_density_unit = self.getPropertyValue('SampleNumberDensityUnit')
        self._sample_density = self.getProperty('SampleDensity').value

        if self._container_ws:
            self._container_material_defined = self.getProperty('ContainerMaterialAlreadyDefined').value
            self._container_chemical_formula = self.getPropertyValue('ContainerChemicalFormula')
            self._container_coherent_cross_section = self.getPropertyValue('ContainerCoherentXSection')
            self._container_incoherent_cross_section = self.getPropertyValue('ContainerIncoherentXSection')
            self._container_attenuation_cross_section = self.getPropertyValue('ContainerAttenuationXSection')
            self._container_density_type = self.getPropertyValue('ContainerDensityType')
            self._container_number_density_unit = self.getPropertyValue('ContainerNumberDensityUnit')
            self._container_density = self.getProperty('ContainerDensity').value

        if self._shape == 'FlatPlate':
            self._sample_width = self.getProperty('SampleWidth').value
            self._sample_thickness = self.getProperty('SampleThickness').value
            self._sample_angle = self.getProperty('SampleAngle').value
            self._sample_center = self.getProperty('SampleCenter').value

        if self._shape == 'Cylinder':
            self._sample_radius = self.getProperty('SampleRadius').value

        if self._shape == 'Annulus':
            self._sample_inner_radius = self.getProperty('SampleInnerRadius').value
            self._sample_outer_radius = self.getProperty('SampleOuterRadius').value

        if self._container_ws:
            if self._shape == 'FlatPlate':
                self._container_front_thickness = self.getProperty('ContainerFrontThickness').value
                self._container_back_thickness = self.getProperty('ContainerBackThickness').value

            if self._shape == 'Cylinder':
                self._container_radius = self.getProperty('ContainerRadius').value

            if self._shape == 'Annulus':
                self._container_inner_radius = self.getProperty('ContainerInnerRadius').value
                self._container_outer_radius = self.getProperty('ContainerOuterRadius').value

        self._output_ws = self.getProperty('CorrectionsWorkspace').value
        output_ws_name = self.getPropertyValue('CorrectionsWorkspace')
        self._ass_ws_name = output_ws_name + "_ass"
        self._acc_ws_name = output_ws_name + "_acc"
        self._assc_ws_name = output_ws_name + "_assc"
        self._acsc_ws_name = output_ws_name + "_acsc"
        self._transposed = False
        self._indirect_elastic = False

        self._set_sample_method = 'Chemical Formula' if self._sample_chemical_formula != '' else 'Cross Sections'
        self._set_can_method = 'Chemical Formula' if self._container_chemical_formula != '' else 'Cross Sections'

    def validateInputs(self):
        issues = dict()

        try:
            self._setup()
        except Exception as err:
            issues['SampleWorkspace'] = str(err)

        if self._shape == 'Annulus':
            if self._sample_inner_radius >= self._sample_outer_radius:
                issues['SampleOuterRadius'] = 'Must be greater than SampleInnerRadius (' \
                                              + str(self._sample_inner_radius) + "). Current value " \
                                              + str(self._sample_outer_radius)

        if self._container_ws:
            container_unit = self._container_ws.getAxis(0).getUnit().unitID()
            if container_unit != self._sample_unit:
                issues['ContainerWorkspace'] = 'Sample and Container workspaces must have the same units.'

            if self._shape == 'Cylinder':
                if self._container_radius <= self._sample_radius:
                    issues['ContainerRadius'] = 'Must be greater than SampleRadius'

            if self._shape == 'Annulus':
                if self._container_inner_radius >= self._sample_inner_radius:
                    issues['ContainerInnerRadius'] = 'Must be less than SampleInnerRadius'
                if self._container_outer_radius <= self._sample_outer_radius:
                    issues['ContainerOuterRadius'] = 'Must be greater than SampleOuterRadius'

        return issues

    def _get_efixed(self):
        """
        Returns the efixed value relating to the specified workspace
        """
        inst = self._sample_ws.getInstrument()

        if inst.hasParameter('Efixed'):
            return inst.getNumberParameter('Efixed')[0]

        if inst.hasParameter('analyser'):
            analyser_comp = inst.getComponentByName(inst.getStringParameter('analyser')[0])

            if analyser_comp is not None and analyser_comp.hasParameter('Efixed'):
                return analyser_comp.getNumberParameter('EFixed')[0]

        raise ValueError('No Efixed parameter found')

    # ------------------------------- Converting to/from wavelength -------------------------------

    def _convert_to_wavelength(self, workspace):
        """
        Converts the specified workspace to units of wavelength.

        :param workspace:   The workspace to convert.
        :return:
        """

        x_unit = workspace.getAxis(0).getUnit().unitID()
        y_unit = workspace.getAxis(1).getUnit().unitID()

        # ----- Quick Conversions (Wavelength and DeltaE) -----

        if x_unit == 'Wavelength':
            return self._clone_ws(workspace)
        elif y_unit == 'Wavelength':
            self._transposed = True
            return self._tranpose_ws(workspace)
        elif x_unit == 'DeltaE':
            return self._convert_units(workspace, "Wavelength", self._emode, self._efixed)
        elif y_unit == 'DeltaE':
            self._transposed = True
            workspace = self._tranpose_ws(workspace)
            return self._convert_units(workspace, "Wavelength", self._emode, self._efixed)

        # ----- Indirect Elastic Conversions -----

        if self._emode == "Indirect":

            if x_unit == 'MomentumTransfer':
                self._transposed = True
                return self._create_waves_indirect_elastic(self._tranpose_ws(workspace))
            else:
                return self._create_waves_indirect_elastic(self._clone_ws(workspace))

        # ----- Direct Conversions -----

        return self._convert_units(workspace, "Wavelength", self._emode)

    def _convert_from_wavelength(self, workspace):
        """
        Converts the specified workspace into units of wavelength.

        :param workspace:   The workspace whose units to convert.
        :return:            A workspace with units of wavelength, created from
                            converting the specified workspace.
        """

        if self._transposed:
            workspace = self._tranpose_ws(workspace)

        if self._sample_unit == 'Label' and not self._isis_instrument:
            # This happens for E/I Fixed Window Scans for IN16B at ILL
            # In this case we want to keep the correction workspace in wavelength and the vertical axis as in the input
            return workspace
        elif self._indirect_elastic:
            return self._convert_units(workspace, "MomentumTransfer", self._emode, self._efixed)
        elif self._emode == "Indirect":
            return self._convert_units(workspace, self._sample_unit, self._emode, self._efixed)
        elif self._sample_unit != 'Wavelength':
            return self._convert_units(workspace, self._sample_unit, self._emode)
        else:
            return workspace

    # ------------------------------- Converting IndirectElastic to wavelength ------------------------------
    def _create_waves_indirect_elastic(self, workspace):
        """
        Creates a wavelength workspace, from the workspace with the specified input workspace
        name, using an Elastic instrument definition file. E-Mode must be Indirect and the y-axis
        of the input workspace must be in units of Q.

        :param workspace:   The input workspace.
        :return:            The output wavelength workspace.
        """
        self._indirect_elastic = True
        self._q_values = workspace.getAxis(1).extractValues()
        instrument_name = workspace.getInstrument().getName()
        self._isis_instrument = instrument_name == "IRIS" or instrument_name == "OSIRIS"

        # ---------- Load Elastic Instrument Definition File ----------

        if self._isis_instrument:
            idf_name = instrument_name + '_elastic_Definition.xml'

            idf_path = os.path.join(config.getInstrumentDirectory(), idf_name)
            logger.information('IDF = %s' % idf_path)

            load_alg = self.createChildAlgorithm("LoadInstrument", enableLogging=True)
            load_alg.setProperty("Workspace", workspace)
            load_alg.setProperty("Filename", idf_path)
            load_alg.setProperty("RewriteSpectraMap", True)
            load_alg.execute()

        e_fixed = float(self._efixed)
        logger.information('Efixed = %f' % e_fixed)

        # ---------- Set Instrument Parameters ----------

        sip_alg = self.createChildAlgorithm("SetInstrumentParameter", enableLogging=False)
        sip_alg.setProperty("Workspace", workspace)
        sip_alg.setProperty("ParameterName", 'EFixed')
        sip_alg.setProperty("ParameterType", 'Number')
        sip_alg.setProperty("Value", str(e_fixed))
        sip_alg.execute()

        # ---------- Calculate Wavelength ----------

        wave = math.sqrt(81.787 / e_fixed)
        logger.information('Wavelength = %f' % wave)
        workspace.getAxis(0).setUnit('Wavelength')

        # ---------- Format Input Workspace ---------

        convert_alg = self.createChildAlgorithm("ConvertToHistogram", enableLogging=False)
        convert_alg.setProperty("InputWorkspace", workspace)
        convert_alg.execute()

        workspace = self._crop_ws(convert_alg.getProperty("OutputWorkspace").value)

        # --------- Set wavelengths as X-values in Output Workspace ----------

        waves = (0.01 * np.arange(-1, workspace.blocksize())) + wave
        logger.information('Waves for the dummy workspace: ' + str(waves))
        nhist = workspace.getNumberHistograms()
        for idx in range(nhist):
            workspace.setX(idx, waves)

        if self._isis_instrument:
            workspace.replaceAxis(1, SpectraAxis.create(workspace))
            self._update_instrument_angles(workspace, self._q_values, wave)

        return workspace

    def _update_instrument_angles(self, workspace, q_values, wave):
        """
        Updates the instrument angles in the specified workspace, using the specified wavelength
        and the specified Q-Values. This is required when calculating absorption corrections for
        indirect elastic. This is used only for ISIS instruments.

        :param workspace:   The workspace whose instrument angles to update.
        :param q_values:    The extracted Q-Values (MomentumTransfer)
        :param wave:        The wavelength
        """
        work_dir = config['defaultsave.directory']
        k0 = 4.0 * math.pi / wave
        theta = 2.0 * np.degrees(np.arcsin(q_values / k0))  # convert to angle

        filename = 'Elastic_angles.txt'
        path = os.path.join(work_dir, filename)
        logger.information('Creating angles file : ' + path)
        handle = open(path, 'w')
        head = 'spectrum,theta'
        handle.write(head + " \n")
        for n in range(0, len(theta)):
            handle.write(str(n + 1) + '   ' + str(theta[n]) + "\n")
        handle.close()

        update_alg = self.createChildAlgorithm("UpdateInstrumentFromFile", enableLogging=False)
        update_alg.setProperty("Workspace", workspace)
        update_alg.setProperty("Filename", path)
        update_alg.setProperty("MoveMonitors", False)
        update_alg.setProperty("IgnorePhi", True)
        update_alg.setProperty("AsciiHeader", head)
        update_alg.setProperty("SkipFirstNLines", 1)

    def _crop_ws(self, workspace):
        """
        Crops the specified workspace to the XMin and XMax values specified in
        it's first and last X-Values.

        :param workspace:   The workspace to crop.
        :return:            The cropped workspace.
        """
        x = workspace.dataX(0)
        xmin = x[0]
        xmax = x[1]
        crop_alg = self.createChildAlgorithm("CropWorkspace", enableLogging=False)
        crop_alg.setProperty("InputWorkspace", workspace)
        crop_alg.setProperty("XMin", xmin)
        crop_alg.setProperty("XMax", xmax)
        crop_alg.execute()
        return crop_alg.getProperty("OutputWorkspace").value

    def _clone_ws(self, input_ws):
        """
        Clones the specified input workspace.

        :param input_ws:    The workspace to clone.
        :return:            A clone of the specified workspace.
        """
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone_alg.setProperty("InputWorkspace", input_ws)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _group_ws(self, workspaces):
        """
        Groups the specified input workspaces.

        :param input_ws:    A list of the workspaces to group together.
        :return:            A WorkspaceGroup containing the specified workspaces.
        """
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", workspaces)
        group_alg.execute()
        return group_alg.getProperty("OutputWorkspace").value

    def _convert_units(self, workspace, target_unit, emode, efixed=None):
        """
        Converts the units of the specified workspace to the target unit, given
        the specified EMode, and potentially EFixed value.

        :param workspace:   The workspace whose units to convert.
        :param target_unit: The unit to convert to.
        :param emode:       The EMode to use in conversion.
        :param efixed:      The EFixed value to use in conversion.
        :return:            A workspace created from converting the units of the
                            specified workspace to the specified target unit.
        """
        convert_units_alg = self.createChildAlgorithm("ConvertUnits", enableLogging=False)
        convert_units_alg.setProperty("InputWorkspace", workspace)
        convert_units_alg.setProperty("Target", target_unit)
        convert_units_alg.setProperty("EMode", emode)
        # Check if EFixed was defined
        if efixed is not None:
            convert_units_alg.setProperty("EFixed", efixed)
        convert_units_alg.execute()
        return convert_units_alg.getProperty("OutputWorkspace").value

    def _convert_spectra_axis(self, workspace, target, emode, efixed):
        """
        Convert the units of the spectra axis of the specified workspace to the
        specified target unit, given the specified EMode and EFixed value.

        :param workspace:   The workspace whose spectra axis to convert.
        :param target:      The target unit to convert to.
        :param emode:       The EMode to use in conversion.
        :param efixed:      The EFixed value to use in conversion.
        :return:            A workspace created from converting the units of the
                            spectra axis of the specified workspace to the specified
                            target unit.
        """
        convert_axis_alg = self.createChildAlgorithm("ConvertSpectraAxis", enableLogging=False)
        convert_axis_alg.setProperty("InputWorkspace", workspace)
        convert_axis_alg.setProperty("EMode", emode)
        convert_axis_alg.setProperty("Target", target)
        convert_axis_alg.setProperty("EFixed", efixed)
        convert_axis_alg.execute()
        return convert_axis_alg.getProperty("OutputWorkspace").value

    def _tranpose_ws(self, workspace):
        """
        Tranposes the specified workspace.

        :param workspace:   The workspace to transpose.
        :return:            The transpose of the specified workspace.
        """
        transpose_alg = self.createChildAlgorithm("Transpose", enableLogging=False)
        transpose_alg.setProperty("InputWorkspace", workspace)
        transpose_alg.execute()
        return transpose_alg.getProperty("OutputWorkspace").value

    # ------------------------------- Utility algorithms -------------------------------
    @staticmethod
    def _set_algorithm_properties(algorithm, properties):
        """
        Sets the specified algorithm's properties using the given properties.

        :param algorithm:   The algorithm whose properties to set.
        :param properties:  The dictionary of properties to set.
        """
        for key, value in properties.items():
            algorithm.setProperty(key, value)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(PaalmanPingsMonteCarloAbsorption)
