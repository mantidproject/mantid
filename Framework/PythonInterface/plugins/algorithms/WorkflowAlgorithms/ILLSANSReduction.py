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
                             defaultValue='Monitor',
                             validator=StringListValidator(['None', 'Timer', 'Monitor']),
                             doc='Choose the normalisation type.')

        self.declareProperty('BeamCenterX', 0., direction=Direction.InOut, doc='Beam center X [m]')

        self.declareProperty('BeamCenterY', 0., direction=Direction.InOut, doc='Beam center Y [m]')

        self.setPropertySettings('BeamCenterX', not_absorber)

        self.setPropertySettings('BeamCenterY', not_absorber)

        self.declareProperty('BeamFluxValue', 1., direction=Direction.InOut, validator=FloatBoundedValidator(lower=0.), doc='Beam flux value')

        self.setPropertySettings('BeamFluxValue',
                                 EnabledWhenProperty(sample, EnabledWhenProperty(beam, reference, LogicOperator.Or), LogicOperator.Or))

        self.declareProperty('BeamFluxError', 0., direction=Direction.InOut, validator=FloatBoundedValidator(lower=0.), doc='Beam flux error')

        self.setPropertySettings('BeamFluxError',
                                 EnabledWhenProperty(sample, EnabledWhenProperty(beam, reference, LogicOperator.Or), LogicOperator.Or))

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
        return grouped

    def _normalise(self, ws):

        normalise_by = self.getPropertyValue('NormaliseBy')
        mon = ws + '_mon'
        ExtractMonitors(InputWorkspace=ws, DetectorWorkspace=ws, MonitorWorkspace=mon)
        if normalise_by == 'Monitor':
            ExtractSingleSpectrum(InputWorkspace=mon, WorkspaceIndex=0, OutputWorkspace=mon)
            if mtd[mon].readY(0)[0] == 0:
                raise RuntimeError('Normalise to monitor requested, but monitor has 0 counts.')
            Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws)
        elif normalise_by == 'Timer':
            if mtd[ws].getRun().hasProperty('timer'):
                duration = mtd[ws].getRun().getLogData('timer').value
                if duration != 0.:
                    Scale(InputWorkspace=ws, Factor=1./duration, OutputWorkspace=ws)
                else:
                    raise RuntimeError('Unable to normalise to time; duration found is 0 seconds.')
            else:
                raise RuntimeError('Normalise to timer requested, but timer information is not available.')
        DeleteWorkspace(mon)

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
        integral = self._integrate_in_radius(ws, radius)
        run = mtd[ws].getRun()
        att_coeff = run.getLogData('attenuator.attenuation_coefficient').value
        l2 = run.getLogData('L2').value
        dx = run.getLogData('pixel_width').value
        dy = run.getLogData('pixel_height').value
        factor = att_coeff * dx * dy / (l2 * l2)
        Scale(InputWorkspace=integral, Factor=factor, OutputWorkspace=integral)
        self.setProperty('BeamFluxValue', mtd[integral].readY(0)[0])
        self.setProperty('BeamFluxError', mtd[integral].readE(0)[0])
        DeleteWorkspace(integral)

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
                if process == 'Transmission':
                    radius = self.getProperty('BeamRadius').value
                    beam = self.getPropertyValue('BeamInputWorkspace')
                    ws_rebin = ws + '_rebin'
                    RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=beam, OutputWorkspace=ws_rebin)
                    transmission = ws + '_tr'
                    shapeXML = self._cylinder(radius)
                    det_list = FindDetectorsInShape(Workspace=ws, ShapeXML=shapeXML)
                    CalculateTransmission(SampleRunWorkspace=ws_rebin, DirectRunWorkspace=beam,
                                          TransmissionROI=det_list, OutputWorkspace=transmission)
                    self.setProperty('TransmissionValue', mtd[transmission].readY(0)[0])
                    self.setProperty('TransmissionError', mtd[transmission].readE(0)[0])
                    DeleteWorkspaces([transmission, ws_rebin])
                else:
                    beam_x = self.getProperty('BeamCenterX').value
                    beam_y = self.getProperty('BeamCenterY').value
                    transmission = self.getProperty('TransmissionValue').value
                    transmission_err = self.getProperty('TransmissionError').value
                    MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')
                    SANSSolidAngleCorrection(InputWorkspace=ws, OutputWorkspace=ws)
                    ApplyTransmissionCorrection(InputWorkspace=ws, TransmissionValue=transmission,
                                                TransmissionError=transmission_err, OutputWorkspace=ws)
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
                                flux = self.getProperty('BeamFluxValue').value
                                ferr = self.getProperty('BeamFluxError').value
                                flux_ws = ws + '_flux'
                                CreateSingleValuedWorkspace(DataValue=flux, ErrorValue=ferr, OutputWorkspace=flux_ws)
                                Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws)
                                DeleteWorkspace(flux_ws)

        CalculateQMinMax(Workspace=ws)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ILLSANSReduction)
