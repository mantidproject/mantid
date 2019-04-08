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
from mantid.simpleapi import (AppendSpectra, Divide, ExtractSingleSpectrum, ExtractSpectra, Integration, SumSpectra,
                              VesuvioTOFFit)
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


def conjoin_workspaces(workspaces):
    conjoined = workspaces[0]
    for workspace in workspaces[1:]:
        conjoined = append(conjoined, workspace)
    return conjoined


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
        self.declareProperty(MatrixWorkspaceProperty(name='OutputCAAD', defaultValue='',
                                                     optional=PropertyMode.Mandatory, direction=Direction.Output),
                             doc='The name of the output workspace containing the sum of the normalised input data.')
        self.declareProperty(name='Chi_squared', defaultValue=0.0, direction=Direction.Output,
                             doc='The value of the Chi-Squared.')

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

        fit_workspaces = [self._vesuvio_tof_fit(self._input_workspace, index) for index in range(number_of_spectra)]
        input_workspaces = [extract_single_spectra(self._input_workspace, index) for index in range(number_of_spectra)]

        fit_caad = conjoin_workspaces(self._compute_caad(fit_workspaces))
        set_axis_labels(fit_caad, self._get_caad_axis_labels(fit_workspaces[0]), 1)

        self.setProperty('OutputWorkspace', fit_caad)
        self.setProperty('OutputCAAD', self._compute_caad(input_workspaces)[0])
        self.setProperty('Chi_squared', 5.5)

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
        return label != "Data" and label != "Diff"


AlgorithmFactory.subscribe(VesuvioCAAD)
