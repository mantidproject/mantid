# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import DataProcessorAlgorithm, WorkspaceProperty, MultipleFileProperty, PropertyMode, \
    FileAction, MatrixWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator, FloatArrayProperty, IntBoundedValidator, StringListValidator
from mantid.simpleapi import *

N_DISTANCES = 5
N_LAMBDAS = 2


class SANSILLMultiProcess(DataProcessorAlgorithm):

    instrument = None # the name of the instrument [D11, D11B, D16, D22, D22B, D33]
    mode = None # the acquisition mode Mono, Kinetic, TOF
    rank = None # the rank of the reduction, i.e. the number of (detector distance, wavelength) configurations
    lambda_rank = None # how many transmissions need to be calculated
    n_samples = None # how many samples
    n_frames = None # how many frames per sample in case of kinetic

    def category(self):
        return 'ILL\\SANS;ILL\\Auto'

    def summary(self):
        return 'Performs SANS data reduction of the entire experiment.'

    def seeAlso(self):
        return ['SANSILLReduction', 'SANSILLIntegration']

    def name(self):
        return 'SANSILLMultiProcess'

    def PyInit(self):

        #================================INPUT RUNS================================#

        for d in range(N_DISTANCES):
            p_name = f'SampleRunsD{d+1}'
            self.declareProperty(MultipleFileProperty(name=p_name,
                                                      action=FileAction.OptionalLoad,
                                                      extensions=['nxs'],
                                                      allow_empty=True),
                                 doc=f'Sample run(s) at the distance #{d+1}.')
            self.setPropertyGroup(p_name, 'Numors')

        self.declareProperty(MultipleFileProperty(name='DarkCurrentRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s).')
        self.setPropertyGroup('DarkCurrentRuns', 'Numors')

        self.declareProperty(MultipleFileProperty(name='EmptyBeamRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s).')
        self.setPropertyGroup('EmptyBeamRuns', 'Numors')

        self.declareProperty(MultipleFileProperty(name='FluxRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s) for flux calculation only; '
                                 'if left blank the flux will be calculated from EmptyBeamRuns.')
        self.setPropertyGroup('FluxRuns', 'Numors')

        self.declareProperty(MultipleFileProperty(name='EmptyContainerRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty container run(s).')
        self.setPropertyGroup('EmptyContainerRuns', 'Numors')

        #================================TR INPUT RUNS================================#

        for l in range(N_LAMBDAS):
            p_name = f'SampleTrRunsW{l+1}'
            self.declareProperty(MultipleFileProperty(name=p_name,
                                                      action=FileAction.OptionalLoad,
                                                      extensions=['nxs'],
                                                      allow_empty=True),
                                 doc=f'Sample run(s) at the wavelength #{l+1}.')
            self.setPropertyGroup(p_name, 'Transmission Numors')

        self.declareProperty(MultipleFileProperty(name='TrCadmiumRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Absorber (Cd/B4C) run(s) for transmission calculation.')

        self.declareProperty(MultipleFileProperty(name='ContainerTrRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Container transmission run(s).')
        self.setPropertyGroup('ContainerTrRuns', 'Transmission Numors')

        self.declareProperty(MultipleFileProperty(name='TrEmptyBeamRuns',
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='Empty beam run(s) for transmission calculation.')

        self.setPropertyGroup('TrCadmiumRuns', 'Transmission Numors')
        self.setPropertyGroup('ContainerTrRuns', 'Transmission Numors')
        self.setPropertyGroup('TrEmptyBeamRuns', 'Transmission Numors')

        #=============================INPUT FILES/WORKSPACES==============================#

        self.declareProperty(name='SensitivityMap', defaultValue='',
                             doc='File or workspace containing the map of the relative detector efficiencies.')
        self.setPropertyGroup('SensitivityMap', 'Input Files/Workspaces')

        self.declareProperty(name='DefaultMask', defaultValue='',
                             doc='File or workspace containing the default mask (detector edges and dead pixels/tubes)'
                                 ' to be applied to all the detector configurations.')
        self.setPropertyGroup('DefaultMask', 'Input Files/Workspaces')

        self.declareProperty(name='BeamStopMasks', defaultValue='',
                             doc='File(s) or workspace(s) containing the detector mask per distance configuration (typically beam stop).')
        self.setPropertyGroup('BeamStopMasks', 'Input Files/Workspaces')

        self.declareProperty(name='FlatFields', defaultValue='',
                             doc='File(s) or workspaces containing the reduced water data (in 2D) for absolute normalisation.')
        self.setPropertyGroup('FlatFields', 'Input Files/Workspaces')

        self.declareProperty(name='Solvents', defaultValue='',
                             doc='File(s) or workspace(s) containing the reduced solvent/buffer data (in 2D) for solvent subtraction.')
        self.setPropertyGroup('Solvents', 'Input Files/Workspaces')

        #==============================REDUCTION PARAMETERS===============================#

        self.declareProperty(name='TrThetaDependent', defaultValue=True,
                             doc='Whether or not to apply the transmission correction in 2theta-dependent way.')
        self.setPropertyGroup('TrThetaDependent', 'Parameters')

        self.declareProperty(name='NormaliseBy', defaultValue=['Monitor'],
                             validator=StringListValidator(['None', 'Time', 'Monitor']))
        self.setPropertyGroup('NormaliseBy', 'Parameters')

        self.declareProperty(FloatArrayProperty(name='TrBeamRadius', values=[1.]),
                             doc='Beam radius [m] used as ROI for transmission calculations.')
        self.setPropertyGroup('TrBeamRadius', 'Parameters')

        self.declareProperty(FloatArrayProperty(name='BeamRadius', values=[1.]),
                             doc='Beam radius [m] used for beam center finding and flux calculations.')
        self.setPropertyGroup('BeamRadius', 'Parameters')

        self.declareProperty(FloatArrayProperty(name='SampleThickness', values=[0.1]),
                             doc='Sample thickness [cm] used for final normalisation.')
        self.setPropertyGroup('SampleThickness', 'Parameters')

        self.declareProperty(name='WaterCrossSection', defaultValue=1.,
                             validator=FloatBoundedValidator(lower=0.),
                             doc='Provide the water cross-section; used only if the absolute scale is done by dividing to water.')
        self.setPropertyGroup('WaterCrossSection', 'Parameters')

        self.declareProperty(name='SensitivityWithOffsets', defaultValue=False,
                             doc='Whether the sensitivity data has been measured with different horizontal offsets (D22 only).')
        self.setPropertyGroup('SensitivityWithOffsets', 'Parameters')

        self.declareProperty(name='StitchReferenceIndex', defaultValue=1,
                             validator=IntBoundedValidator(lower=0),
                             doc='The index of the reference workspace during stitching, '
                                 'by default the middle distance will be chosen as reference if there are 3.')
        self.setPropertyGroup('StitchReferenceIndex', 'Parameters')

        self.declareProperty(name='ClearCorrectedRealSpaceWorkspace', defaultValue=False,
                             doc='Whether to clear the fully corrected real-space workspace.')
        self.setPropertyGroup('ClearCorrectedRealSpaceWorkspace', 'Parameters')

        self.declareProperty(name='OutputType', defaultValue='I(Q)',
                             validator=StringListValidator(['I(Q)']),
                             doc='The type of the integration to perform.')
        self.setPropertyGroup('OutputType', 'Parameters')

        #===================================I(Q) OPTIONS==================================#

        self.declareProperty(name='OutputBinning', defaultValue='',
                             doc='Output binning for each distance( : separated list of binning params).')
        self.setPropertyGroup('OutputBinning', 'I(Q) Options')

        iq_options = ['CalculateResolution', 'DefaultQBinning', 'BinningFactor', 'NumberOfWedges', 'WedgeAngle',
                      'WedgeOffset', 'AsymmetricWedges', 'WavelengthRange', 'ShapeTable']
        self.copyProperties('SANSILLIntegration', iq_options)
        for opt in iq_options:
            self.setPropertyGroup(opt, 'I(Q) Options')

        #================================OUTPUT WORKSPACES================================#

        self.declareProperty(WorkspaceProperty(name='OutputWorkspace', defaultValue='',
                                               direction=Direction.Output),
                             doc='The output workspace containing the reduced data.')
        self.setPropertyGroup('OutputWorkspace', 'Output Workspaces')

        self.declareProperty(WorkspaceProperty(name='CorrectedRealSpaceOutputWorkspace', defaultValue='',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='The output workspace containing the fully corrected, but not integrated data in real space.')
        self.setPropertyGroup('CorrectedRealSpaceOutputWorkspace', 'Output Workspaces')

        self.declareProperty(WorkspaceProperty(name='WedgesOutputWorkspace', defaultValue='',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='The output workspace containing the reduced data per azimuthal sector.')
        self.setPropertyGroup('WedgesOutputWorkspace', 'Output Workspaces')

        self.declareProperty(WorkspaceProperty(name='PanelsOutputWorkspace', defaultValue='',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='The output workspace containing the reduced data per detector bank.')
        self.setPropertyGroup('PanelsOutputWorkspace', 'Output Workspaces')

        self.declareProperty(MatrixWorkspaceProperty(name='SensitivityOutputWorkspace', defaultValue='',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output sensitivity map workspace.')
        self.setPropertyGroup('SensitivityOutputWorkspace', 'Output Workspaces')

    def PyExec(self):
        pass


AlgorithmFactory.subscribe(SANSILLMultiProcess)
