# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, division, print_function
import numpy as np
from scipy import constants
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction
import collections
from mantid.simpleapi import CreateWorkspace


class CalculatePlaczekSelfScattering(PythonAlgorithm):
    _input_ws = None
    _output_ws = None

    def category(self):
        return 'Diffraction\\Fitting'

    def name(self):
        return 'FitIncidentSpectrum'

    def summary(self):
        return 'Calculate a fit for an incident spectrum using different methods. ' \
               'Outputs a workspace containing the functionalized fit and its first ' \
               'derivative.'

    def seeAlso(self):
        return [""]

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty('InputWorkspace', '',
                                    direction=Direction.Input,
                                    ),
            doc='')

        self.declareProperty(
            MatrixWorkspaceProperty('OutputWorkspace', '',
                                    direction=Direction.Output),
            doc='')

    def validateInputs(self):
        issues = dict()
        input_ws = self.getProperty('InputWorkspace').value
        det_info = input_ws.detectorInfo()
        if not det_info.size() > 0:
            issues['InputWorkspace'] = 'Workspace has no detector information'
        return issues
        material = input_ws.sample().getMaterial().chemicalFormula()
        if not len(material > 0):
            issues['InputWorkspace'] = 'Workspace has no sample'


    def _setup(self):
        self._input_ws = self.getProperty('InputWorkspace').value
        self._output_ws = self.getProperty('OutputWorkspace').valueAsStr
        self._det_info = self._input_ws.detectorInfo()
        self._L1 = self._det_info.l1()

    def PyExec(self):
        self._setup()
        factor = 1. / constants.physical_constants['atomic mass unit-kilogram relationship'][0]
        neutron_mass = factor * constants.m_n

        # get sample information: mass, total scattering length, and concentration
        # of each species
        atom_species = self.get_sample_species_info()
        # calculate summation term w/ neutron mass over molecular mass ratio
        summation_term = 0.0
        for species, props in atom_species.items():
            # inefficient in py2, but works with py3
            summation_term += props['concentration'] * \
                props['b_sqrd_bar'] * neutron_mass / props['mass']

        # get incident spectrum and 1st derivative
        incident_index = 0
        incident_prime_index = 1

        x_lambda = self._input_ws.readX(incident_index)
        incident = self._input_ws.readY(incident_index)
        incident_prime = self._input_ws.readY(incident_prime_index)

        phi_1 = x_lambda * incident_prime / incident

        # Set default Detector Law
        detector = None
        if detector is None:
            detector = {'Alpha': None,
                        'LambdaD': 1.44,
                        'Law': '1/v'}

        # Set detector exponential coefficient alpha
        if detector['Alpha'] is None:
            detector['Alpha'] = 2. * np.pi / detector['LambdaD']

        # Detector law to get eps_1(lambda)
        if detector['Law'] == '1/v':
            c = -detector['Alpha'] / (2. * np.pi)
            x = x_lambda
            detector_law_term = c * x * np.exp(c * x) / (1. - np.exp(c * x))

        eps_1 = detector_law_term

        # Placzek
        '''
        Original Placzek inelastic correction Ref (for constant wavelength, reactor source):
            Placzek, Phys. Rev v86, (1952), pp. 377-388
        First Placzek correction for time-of-flight, pulsed source (also shows reactor eqs.):
            Powles, Mol. Phys., v6 (1973), pp.1325-1350
        Nomenclature and calculation for this program follows Ref:
             Howe, McGreevy, and Howells, J. Phys.: Condens. Matter v1, (1989), pp. 3433-3451
        NOTE: Powles's Equation for inelastic self-scattering is equal to Howe's Equation for P(theta)
        by adding the elastic self-scattering
        '''
        x_lambdas = np.array([])
        placzek_correction = np.array([])
        num_spec = 0
        for i in np.arange(1000):  # np.arange(self._det_info.size()):
            if not self._det_info.isMonitor(i):
                num_spec += 1
                # variables
                path_length = self._L1 + self._det_info.l2(i)
                f = self._L1 / path_length

                angle_conv = np.pi / 180.
                sin_theta_by_2 = np.sin(self._det_info.twoTheta(i) * angle_conv / 2.)

                term1 = (f - 1.) * phi_1
                term2 = f * eps_1
                term3 = f - 3.

                # per_bank_q = ConvertLambdaToQ(x_lambda,theta) - See Eq. (A1.14) of
                inelastic_placzek_self_correction = 2. * \
                    (term1 - term2 + term3) * sin_theta_by_2 * sin_theta_by_2 * summation_term
                x_lambdas = np.append(x_lambdas, x_lambda)
                placzek_correction = np.append(
                    placzek_correction,
                    inelastic_placzek_self_correction)

        output_workspace = CreateWorkspace(
            DataX=x_lambdas,
            DataY=placzek_correction,
            UnitX='Wavelength',
            NSpec=num_spec,
            ParentWorkspace=self._input_ws,
            Distribution=True)
        self.setProperty("OutputWorkspace", output_workspace)

    def get_sample_species_info(self):
        # get sample information: mass, total scattering length, and concentration
        # of each species
        total_stoich = 0.0
        material = self._input_ws.sample().getMaterial().chemicalFormula()
        atom_species = collections.OrderedDict()
        for atom, stoich in zip(material[0], material[1]):
            # <b^2> == sigma_s / 4*pi (in barns)
            b_sqrd_bar = self._input_ws.sample().getMaterial().totalScatterXSection() / (4. * np.pi)
            atom_species[atom.symbol] = {'mass': atom.mass,
                                         'stoich': stoich,
                                         'b_sqrd_bar': b_sqrd_bar}
            total_stoich += stoich

        for atom, props in atom_species.items():
            # inefficient in py2, but works with py3
            props['concentration'] = props['stoich'] / total_stoich

        return atom_species


# AlgorithmFactory.subscribe(CalculatePlaczekSelfScattering)
