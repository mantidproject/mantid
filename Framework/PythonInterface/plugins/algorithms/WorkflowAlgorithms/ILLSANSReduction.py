from __future__ import (absolute_import, division, print_function)

from mantid.api import DataProcessorAlgorithm, MultipleFileProperty, FileProperty, \
    WorkspaceGroupProperty, FileAction, Progress, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import StringListValidator, Direction, EnabledWhenProperty, PropertyCriterion, LogicOperator, FloatBoundedValidator
from mantid.simpleapi import *


class ILLSANSReduction(DataProcessorAlgorithm):

    def category(self):
        return "ILL\\SANS"

    def summary(self):
        return 'Performs SANS data reduction at the ILL.'

    def seeAlso(self):
        return []

    def name(self):
        return "ILLSANSReduction"

    def validateInputs(self):
        return dict()

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run (s).')

        options = ['Absorber', 'Beam', 'Transmission', 'Container', 'Reference', 'Sample']

        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(options),
                             doc='Choose the process type.')

        not_absorber = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsNotEqualTo, 'Absorber')

        sample = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Sample')

        beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Beam')

        transmission = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Transmission')

        not_beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsNotEqualTo, 'Beam')

        reference = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Reference')

        self.declareProperty(name='NormaliseBy',
                             defaultValue='None',
                             validator=StringListValidator(['None', 'Timer', 'Monitor']),
                             doc='Choose the normalisation type.')

        self.declareProperty('BeamCenterX', 0., direction=Direction.InOut, doc='Beam center X [m]')

        self.declareProperty('BeamCenterY', 0., direction=Direction.InOut, doc='Beam center Y [m]')

        self.setPropertySettings('BeamCenterX', not_absorber)

        self.setPropertySettings('BeamCenterY', not_absorber)

        self.declareProperty('BeamFlux', 1., direction=Direction.InOut, validator=FloatBoundedValidator(lower=0.), doc='Beam flux')

        self.setPropertySettings('BeamFlux',
                                 EnabledWhenProperty(beam, EnabledWhenProperty(beam, reference, LogicOperator.Or), LogicOperator.Or))

        self.declareProperty('BeamRadius', 0.05, validator=FloatBoundedValidator(lower=0.), doc='Beam raduis [m]')

        self.setPropertySettings('BeamRadius',
                                 EnabledWhenProperty(beam, transmission, LogicOperator.Or))

        self.declareProperty('DirectBeam', True, doc='Use direct beam method of the beam finding.')

        self.setPropertySettings('DirectBeam', beam)

        self.declareProperty('TransmissionValue', 1., direction=Direction.InOut, doc='Transmission value')

        self.setPropertySettings('TransmissionValue',
                                 EnabledWhenProperty(not_absorber, not_beam, LogicOperator.And))

        self.declareProperty('TransmissionError', 0., direction=Direction.InOut, doc='Transmission error')

        self.setPropertySettings('TransmissionError',
                                 EnabledWhenProperty(not_absorber, not_beam, LogicOperator.And))

        self.declareProperty('SampleThickness', 0.1, validator=FloatBoundedValidator(lower=0.), doc='Sample thickness [cm]')

        self.setPropertySettings('SampleThickness', EnabledWhenProperty(sample, reference, LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='The output workspace.')

        self.declareProperty(MatrixWorkspaceProperty('AbsorberInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the absorber workspace.')

        self.setPropertySettings('AbsorberInputWorkspace', not_absorber)

        self.declareProperty(MatrixWorkspaceProperty('BeamInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the empty beam workspace.')

        self.setPropertySettings('BeamInputWorkspace',
                                 EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Transmission'))

        self.declareProperty(MatrixWorkspaceProperty('ContainerInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the container workspace.')

        self.setPropertySettings('ContainerInputWorkspace',
                                 EnabledWhenProperty(sample, reference, LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('SensitivityInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the input sensitivity workspace.')

        self.setPropertySettings('SensitivityInputWorkspace', sample)

        self.declareProperty(MatrixWorkspaceProperty('SensitivityOutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the output sensitivity workspace.')

        self.setPropertySettings('SensitivityOutputWorkspace', reference)

        self.declareProperty(MatrixWorkspaceProperty('ReferenceInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the reference workspace.')

        self.setPropertySettings('ReferenceInputWorkspace', sample)

        self.declareProperty(MatrixWorkspaceProperty('MaskedInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Workspace to copy the mask from; for example, the beam stop')

    def _cylinder(self, radius):

        return '<infinite-cylinder id="flux"><centre x="0.0" y="0.0" z="0.0"/><axis x="0.0" y="0.0" z="1.0"/><radius val="{0}"/></infinite-cylinder>'.format(radius)

    def _integrate_in_radius(self, ws, radius):

        shapeXML = self._cylinder(radius)
        det_list = FindDetectorsInShape(Workspace=ws, ShapeXML=shapeXML)
        grouped = ws + '_group'
        GroupDetectors(InputWorkspace=ws, OutputWorkspace=grouped, DetectorList=det_list)
        sum = mtd[grouped].readY(0)[0]
        DeleteWorkspace(grouped)
        return sum

    def _normalise(self, ws):

        normalise_by = self.getPropertyValue('NormaliseBy')
        if normalise_by == 'Monitor':
            NormaliseToMonitor(InputWorkspace=ws, MonitorID=100000, OutputWorkspace=ws)
        elif normalise_by == 'Timer':
            if mtd[ws].getRun().hasProperty('Timer'):
                duration = mtd[ws].getRun().getLogData('Timer').value
                if duration != 0.:
                    Scale(InputWorkspace=ws, Factor=1./duration, OutputWorkspace=ws)
                else:
                    raise RuntimeError('Unable to normalise to time; duration found is 0 seconds.')
            else:
                raise RuntimeError('Normalise to timer requested, but timer information is not available.')
        ExtractMonitors(InputWorkspace=ws, DetectorWorkspace=ws)

    def _process_beam(self, ws):

        centers = ws + '_centers'
        method = self.getProperty('DirectBeam').value
        radius = self.getProperty('BeamRadius').value
        FindCenterOfMassPosition(InputWorkspace=ws, DirectBeam=method, BeamRadius=radius, Output=centers)
        beam_x = mtd[centers].cell(0,1)
        beam_y = mtd[centers].cell(1,1)
        self.setProperty('BeamCenterX', beam_x)
        self.setProperty('BeamCenterY', beam_y)
        DeleteWorkspace(centers)
        flux = self._integrate_in_radius(ws, radius)
        att_coeff = mtd[ws].getRun().getLogData('attenuator.attenuation_coefficient').value
        flux *= att_coeff # * (dx * dy/L)2
        self.setProperty('BeamFlux', flux)

    def PyExec(self):

        process = self.getPropertyValue('ProcessAs')
        ws = '__' + self.getPropertyValue('OutputWorkspace')
        LoadAndMerge(Filename=self.getPropertyValue('Run').replace(',','+'),
                     LoaderName='LoadILLSANS', OutputWorkspace=ws)
        self._normalise(ws)
        if process != 'Absorber':
            absorber_ws = self.getPropertyValue('AbsorberInputWorkspace')
            if absorber_ws:
                Minus(LHSWorkspace=ws, RHSWorkspace=absorber_ws, OutputWorkspace=ws)
            if process == 'Beam':
                self._process_beam(ws)
            else:
                beam_x = self.getProperty('BeamCenterX').value
                beam_y = self.getProperty('BeamCenterY').value
                MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')
                SANSSolidAngleCorrection(InputWorkspace=ws, OutputWorkspace=ws)
                if process == 'Transmission':
                    radius = self.getProperty('BeamRadius').value
                    beam = self.getPropertyValue('BeamInputWorkspace')
                    empty = self._integrate_in_radius(beam, radius)
                    scatterer = self._integrate_in_radius(ws, radius)
                    self.setProperty('TransmissionValue', scatterer/empty)
                else:
                    transmission = self.getProperty('TransmissionValue').value
                    ApplyTransmissionCorrection(InputWorkspace=ws, TransmissionValue=transmission, OutputWorkspace=ws)
                    if process != 'Container':
                        container = self.getPropertyValue('ContainerInputWorkspace')
                        if container:
                            Minus(LHSWorkspace=ws, RHSWorkspace=container, OutputWorkspace=ws)
                        mask = self.getPropertyValue('MaskedInputWorkspace')
                        if mask:
                            MaskDetectors(Workspace=ws, MaskedWorkspace=mask)
                        if process == 'Reference':
                            sensitivity_out = self.getPropertyValue('SensitivityOutputWorkspace')
                            if sensitivity_out:
                                CalculateEfficiency(InputWorkspace=ws, OutputWorkspace=sensitivity_out)
                                self.setProperty('SensitivityOutputWorkspace', mtd[sensitivity_out])
                        elif process == 'Sample':
                            sensitivity_in = self.getPropertyValue('SensitivityInputWorkspace')
                            if sensitivity_in:
                                Divide(LHSWorkspace=ws, RHSWorkspace=sensitivity_in, OutputWorkspace=ws)
                            thickness = self.getProperty('SampleThickness').value
                            NormaliseByThickness(InputWorkspace=ws, OutputWorkspace=ws, SampleThickness=thickness)
                            reference = self.getPropertyValue('ReferenceInputWorkspace')
                            if reference:
                                pass
                            else:
                                flux = self.getProperty('BeamFlux').value
                                Scale(InputWorkspace=ws, OutputWorkspace=ws, Factor=1./flux)

        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ILLSANSReduction)
