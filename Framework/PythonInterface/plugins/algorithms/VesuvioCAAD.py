# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode, PythonAlgorithm, TextAxis,
                        WorkspaceGroup, WorkspaceGroupProperty)
from mantid.kernel import (Direction, FloatArrayLengthValidator, FloatArrayProperty, IntBoundedValidator,
                           StringMandatoryValidator)
from mantid.simpleapi import (AppendSpectra, Divide, EvaluateFunction, ExtractSingleSpectrum, ExtractSpectra,
                              Integration, Minus, Multiply, SumSpectra, VesuvioTOFFit)
from functools import reduce


def sum_spectra(workspace):
    return SumSpectra(InputWorkspace=workspace,
                      OutputWorkspace="__summed",
                      StoreInADS=False,
                      EnableLogging=False)


def append(workspace1, workspace2):
    return AppendSpectra(InputWorkspace1=workspace1,
                         InputWorkspace2=workspace2,
                         OutputWorkspace="__appended",
                         StoreInADS=False,
                         EnableLogging=False)


def append_all(workspaces):
    return workspaces[0] if len(workspaces) == 1 else reduce(append, workspaces[1:], workspaces[0])


def extract_single_spectra(workspace, index):
    return ExtractSingleSpectrum(InputWorkspace=workspace,
                                 OutputWorkspace="__extracted",
                                 WorkspaceIndex=index,
                                 StoreInADS=False,
                                 EnableLogging=False)


def extract_spectra(workspace, indices):
    return ExtractSpectra(InputWorkspace=workspace,
                          WorkspaceIndexList=indices,
                          OutputWorkspace="__extracted",
                          StoreInADS=False,
                          EnableLogging=False)


def normalise_by_integral(workspace):
    integrated = Integration(InputWorkspace=workspace,
                             OutputWorkspace="__integral",
                             StoreInADS=False,
                             EnableLogging=False)
    return Divide(LHSWorkspace=workspace,
                  RHSWorkspace=integrated,
                  OutputWorkspace="__divided",
                  StoreInADS=False,
                  EnableLogging=False)


def set_axis_labels(workspace, labels, axis_index):
    axis = TextAxis.create(len(labels))
    for index, label in enumerate(labels):
        axis.setLabel(index, label)
    workspace.replaceAxis(axis_index, axis)


class VesuvioCAAD(PythonAlgorithm):
    _input_workspace = None
    _masses = None
    _mass_profiles = None

    _background = None
    _intensity_constraints = None
    _max_iterations = None
    _minimizer = None

    _output_workspace = None
    _chi2 = None

    def summary(self):
        return 'Calculates cumulative angle-averaged data (CAAD) for the VESUVIO instrument.'

    def category(self):
        return 'Inelastic\\Indirect\\Vesuvio'

    def PyInit(self):
        # Input properties
        self.declareProperty(MatrixWorkspaceProperty(name='InputWorkspace', defaultValue='',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc='The input workspace containing TOF data.')

        float_length_validator = FloatArrayLengthValidator()
        float_length_validator.setLengthMin(1)
        self.declareProperty(FloatArrayProperty(name="Masses", validator=float_length_validator),
                             doc="The mass values for fitting.")

        self.declareProperty(name='MassProfiles', defaultValue='', validator=StringMandatoryValidator(),
                             doc='The functions used to approximate the mass profile. The format is '
                                 'function=Function1Name,param1=val1,param2=val2;function=Function2Name,param3=val3,'
                                 'param4=val4')

        self.declareProperty(name="MaxIterations", defaultValue=0, validator=IntBoundedValidator(lower=0),
                             doc="Maximum number of fitting iterations.")
        self.declareProperty(name='Background', defaultValue='',
                             doc='The function used to fit the background. The format is function=FunctionName,'
                                 'param1=val1,param2=val2')
        self.declareProperty(name='IntensityConstraints', defaultValue='',
                             doc='A semi-colon separated list of intensity constraints defined as lists e.g '
                                 '[0,1,0,-4];[1,0,-2,0]')

        self.declareProperty(name='Minimizer', defaultValue='Levenberg-Marquardt,AbsError=1e-08,RelError=1e-08',
                             doc='String defining the minimizer')

        # Output properties
        self.declareProperty(MatrixWorkspaceProperty(name='OutputWorkspace', defaultValue='',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc='The name of the output workspace containing the sum of the normalised fit data.')
        self.declareProperty(MatrixWorkspaceProperty(name='OutputDataCAAD', defaultValue='',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc='The name of the output workspace containing the sum of the normalised input data.')
        self.declareProperty(MatrixWorkspaceProperty(name='OutputChiSquared', defaultValue='',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The output workspace containing the calculated Chi-Squared.')

    def _setup(self):
        self._input_workspace = self.getProperty('InputWorkspace').value
        self._masses = self.getPropertyValue('Masses')
        self._mass_profiles = self.getPropertyValue('MassProfiles')

        self._max_iterations = self.getPropertyValue('MaxIterations')
        self._background = self.getPropertyValue('Background')
        self._intensity_constraints = self.getPropertyValue('IntensityConstraints')
        self._minimizer = self.getPropertyValue('Minimizer')

    def PyExec(self):
        self._setup()

        number_of_spectra = self._input_workspace.getNumberHistograms()

        input_workspaces = [extract_single_spectra(self._input_workspace, index) for index in range(number_of_spectra)]
        data_caad = self._compute_caad(input_workspaces)[0]

        fit_workspaces = [self._vesuvio_tof_fit(self._input_workspace, index) for index in range(number_of_spectra)]
        fit_caad = self._compute_caad(fit_workspaces)[0]
        set_axis_labels(fit_caad, self._get_caad_axis_labels(fit_workspaces[0]), 1)

        #function = 'composite=ComptonScatteringCountRate,NumDeriv=1,IntensityConstraints="Matrix(1|4)0.000000|1.000000|' \
        #           '0.000000|-4.000000";name=GramCharlierComptonProfile,Mass=1.007900,HermiteCoeffs=1 0 0,Width=5.000000;' \
        #           'name=GaussianComptonProfile,Mass=16.000000,Width=10.000000;name=GaussianComptonProfile,Mass=27.000000,' \
        #           'Width=13.000000;name=GaussianComptonProfile,Mass=133.000000,Width=30.000000;name=Polynomial,n=3'

        #function = 'name=Lorentzian,Amplitude=39.1348,PeakCentre=314.934,FWHM=29.5195;name=Lorentzian,Amplitude=113.69' \
        #           '4,PeakCentre=365.873,FWHM=21.2816'
        function = 'name=GaussianComptonProfile,Mass=16.000000,Width=10.000000;name=GaussianComptonProfile,Mass=27.000000,' \
                   'Width=13.000000;name=GaussianComptonProfile,Mass=133.000000,Width=30.000000;'

        output_test = EvaluateFunction(InputWorkspace=data_caad, Function=function)

        self.setProperty('OutputWorkspace', output_test)
        self.setProperty('OutputDataCAAD', data_caad)
        self.setProperty('OutputChiSquared', self._compute_chi_squared(data_caad, fit_caad))

    def _vesuvio_tof_fit(self, workspace, index):
        fitted_workspace, _, _ = VesuvioTOFFit(InputWorkspace=workspace,
                                               Masses=self._masses,
                                               MassProfiles=self._mass_profiles,
                                               Background=self._background,
                                               IntensityConstraints=self._intensity_constraints,
                                               Minimizer=self._minimizer,
                                               MaxIterations=self._max_iterations,
                                               WorkspaceIndex=index,
                                               StoreInADS=False,
                                               EnableLogging=True)
        return fitted_workspace

    def _compute_caad(self, fit_workspaces):
        indices = self._get_caad_indices(fit_workspaces[0])
        normalised_workspaces = [normalise_by_integral(extract_spectra(workspace, indices))
                                 for workspace in fit_workspaces]

        number_of_spectrum = normalised_workspaces[0].getNumberHistograms()
        normalised_workspaces = [[extract_single_spectra(workspace, index) for workspace in normalised_workspaces]
                                 for index in range(number_of_spectrum)]
        normalised_workspaces = [append_all(workspaces) for workspaces in normalised_workspaces]

        summed_workspaces = [sum_spectra(workspace) for workspace in normalised_workspaces]
        return summed_workspaces

    def _get_caad_indices(self, fit_workspace):
        axis_labels = fit_workspace.getAxis(1).extractValues()
        return [index for index, label in enumerate(axis_labels) if self._is_valid_for_caad(label)]

    def _get_caad_axis_labels(self, fit_workspace):
        axis_labels = fit_workspace.getAxis(1).extractValues()
        return [label for label in axis_labels if self._is_valid_for_caad(label)]

    @staticmethod
    def _is_valid_for_caad(label):
        return label == 'Calc'

    @staticmethod
    def _compute_chi_squared(data_caad, fit_caad):
        subtracted = Minus(LHSWorkspace=data_caad,
                           RHSWorkspace=fit_caad,
                           StoreInADS=False,
                           EnableLogging=False)
        return Multiply(LHSWorkspace=subtracted,
                        RHSWorkspace=subtracted,
                        StoreInADS=False,
                        EnableLogging=False)


AlgorithmFactory.subscribe(VesuvioCAAD)
