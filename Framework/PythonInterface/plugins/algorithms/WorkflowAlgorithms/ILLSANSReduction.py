from __future__ import (absolute_import, division, print_function)

from mantid.api import DataProcessorAlgorithm, MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, LogicOperator, PropertyCriterion, StringListValidator
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
        issues = dict()
        if 'ProcessAs' == 'Transmission':
            beam = self.getProperty('BeamInputWorkspace')
            if not beam:
                issues['BeamInputWorkspace'] = 'Beam workspace is mandatory for transmission calculation.'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run (s).')

        options = ['Absorber', 'Beam', 'Transmission', 'Container', 'Reference', 'Sample']

        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(options),
                             doc='Choose the process type.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='The output workspace.')

        not_absorber = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsNotEqualTo, 'Absorber')

        sample = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Sample')

        beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Beam')

        transmission = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Transmission')

        not_beam = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsNotEqualTo, 'Beam')

        reference = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Reference')

        container = EnabledWhenProperty('ProcessAs', PropertyCriterion.IsEqualTo, 'Container')

        self.declareProperty(name='NormaliseBy',
                             defaultValue='Timer',
                             validator=StringListValidator(['None', 'Timer', 'Monitor']),
                             doc='Choose the normalisation type.')

        self.declareProperty('BeamRadius', 0.05, validator=FloatBoundedValidator(lower=0.), doc='Beam raduis [m]')

        self.setPropertySettings('BeamRadius',
                                 EnabledWhenProperty(beam, transmission, LogicOperator.Or))

        self.declareProperty('DirectBeam', True, doc='Use direct beam method of the beam finding.')

        self.setPropertySettings('DirectBeam', beam)

        self.declareProperty('SampleThickness', 0.1, validator=FloatBoundedValidator(lower=0.), doc='Sample thickness [cm]')

        self.setPropertySettings('SampleThickness', EnabledWhenProperty(sample, reference, LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('AbsorberInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the absorber workspace.')

        self.setPropertySettings('AbsorberInputWorkspace', not_absorber)

        self.declareProperty(MatrixWorkspaceProperty('BeamInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the empty beam input workspace.')

        self.setPropertySettings('BeamInputWorkspace',
                                 EnabledWhenProperty(not_absorber, not_beam, LogicOperator.And))

        self.declareProperty(MatrixWorkspaceProperty('TransmissionInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the transmission input workspace.')

        self.setPropertySettings('TransmissionInputWorkspace',
                                 EnabledWhenProperty(container,
                                                     EnabledWhenProperty(reference, sample, LogicOperator.Or),
                                                     LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('ContainerInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the container workspace.')

        self.setPropertySettings('ContainerInputWorkspace',
                                 EnabledWhenProperty(sample, reference, LogicOperator.Or))

        self.declareProperty(MatrixWorkspaceProperty('ReferenceInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the reference workspace.')

        self.setPropertySettings('ReferenceInputWorkspace', sample)

        self.declareProperty(MatrixWorkspaceProperty('SensitivityInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the input sensitivity workspace.')

        self.setPropertySettings('SensitivityInputWorkspace',
                                 EnabledWhenProperty(sample,
                                                     EnabledWhenProperty('ReferenceInputWorkspace',
                                                                         PropertyCriterion.IsEqualTo, ''),
                                                     LogicOperator.And))

        self.declareProperty(MatrixWorkspaceProperty('SensitivityOutputWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the output sensitivity workspace.')

        self.setPropertySettings('SensitivityOutputWorkspace', reference)

        self.declareProperty(MatrixWorkspaceProperty('MaskedInputWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Workspace to copy the mask from; for example, the beam stop')

        self.setPropertySettings('MaskedInputWorkspace', EnabledWhenProperty(sample, reference, LogicOperator.Or))

    def _cylinder(self, radius):
        """
            Returns XML for an infinite cylinder with axis of z (beam) and given radius [m]
            @oaram radius : the radius of the cylinder [m]
            @return : XML string for the geometry shape
        """

        return '<infinite-cylinder id="flux"><centre x="0.0" y="0.0" z="0.0"/><axis x="0.0" y="0.0" z="1.0"/>' \
               '<radius val="{0}"/></infinite-cylinder>'.format(radius)

    def _integrate_in_radius(self, ws, radius):
        """
            Sums the detector counts within the given radius around the beam
            @param ws : the input workspace
            @param radius : the radius [m]
            @returns : the workspace with the summed counts
        """

        shapeXML = self._cylinder(radius)
        det_list = FindDetectorsInShape(Workspace=ws, ShapeXML=shapeXML)
        grouped = ws + '_group'
        GroupDetectors(InputWorkspace=ws, OutputWorkspace=grouped, DetectorList=det_list)
        return grouped

    def _normalise(self, ws):
        """
            Normalizes the workspace by time (SampleLog Timer) or Monitor (ID=100000)
            @param ws : the input workspace
        """

        normalise_by = self.getPropertyValue('NormaliseBy')
        if normalise_by == 'Monitor':
            mon = ws + '_mon'
            ExtractSpectra(InputWorkspace=ws, DetectorList=100000, OutputWorkspace=mon)
            if mtd[mon].readY(0)[0] == 0:
                raise RuntimeError('Normalise to monitor requested, but monitor has 0 counts.')
            else:
                Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws)
                DeleteWorkspace(mon)
        elif normalise_by == 'Timer':
            if mtd[ws].getRun().hasProperty('timer'):
                duration = mtd[ws].getRun().getLogData('timer').value
                if duration != 0.:
                    Scale(InputWorkspace=ws, Factor=1./duration, OutputWorkspace=ws)
                else:
                    raise RuntimeError('Unable to normalise to time; duration found is 0 seconds.')
            else:
                raise RuntimeError('Normalise to timer requested, but timer information is not available.')

    def _process_beam(self, ws):
        """
            Calculates the beam center's x,y coordinates, and the beam flux
            @param ws : the input [empty beam] workspace
        """

        centers = ws + '_centers'
        method = self.getProperty('DirectBeam').value
        radius = self.getProperty('BeamRadius').value
        FindCenterOfMassPosition(InputWorkspace=ws, DirectBeam=method, BeamRadius=radius, Output=centers)
        beam_x = mtd[centers].cell(0,1)
        beam_y = mtd[centers].cell(1,1)
        AddSampleLog(Workspace=ws, LogName='BeamCenterX', LogText=str(beam_x), LogType='Number')
        AddSampleLog(Workspace=ws, LogName='BeamCenterY', LogText=str(beam_y), LogType='Number')
        DeleteWorkspace(centers)
        integral = self._integrate_in_radius(ws, radius)
        run = mtd[ws].getRun()
        att_coeff = run.getLogData('attenuator.attenuation_coefficient').value
        Scale(InputWorkspace=integral, Factor=att_coeff, OutputWorkspace=integral)
        AddSampleLog(Workspace=ws, LogName='BeamFluxValue', LogText=str(mtd[integral].readY(0)[0]), LogType='Number')
        AddSampleLog(Workspace=ws, LogName='BeamFluxError', LogText=str(mtd[integral].readE(0)[0]), LogType='Number')
        DeleteWorkspace(integral)

    def PyExec(self):

        process = self.getPropertyValue('ProcessAs')
        ws = '__' + self.getPropertyValue('OutputWorkspace')
        LoadAndMerge(Filename=self.getPropertyValue('Run').replace(',','+'), LoaderName='LoadILLSANS', OutputWorkspace=ws)
        self._normalise(ws)
        if process != 'Absorber':
            absorber_ws = self.getPropertyValue('AbsorberInputWorkspace')
            if absorber_ws:
                Minus(LHSWorkspace=ws, RHSWorkspace=absorber_ws, OutputWorkspace=ws)
            if process == 'Beam':
                self._process_beam(ws)
            else:
                beam = self.getPropertyValue('BeamInputWorkspace')
                if process == 'Transmission':
                    radius = self.getProperty('BeamRadius').value
                    RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=beam, OutputWorkspace=ws)
                    shapeXML = self._cylinder(radius)
                    det_list = FindDetectorsInShape(Workspace=ws, ShapeXML=shapeXML)
                    CalculateTransmission(SampleRunWorkspace=ws, DirectRunWorkspace=beam,
                                          TransmissionROI=det_list, OutputWorkspace=ws)
                else:
                    if beam:
                        beam_x = mtd[beam].getRun().getLogData('BeamCenterX').value
                        beam_y = mtd[beam].getRun().getLogData('BeamCenterY').value
                        MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')
                    transmission_ws = self.getPropertyValue('TransmissionInputWorkspace')

                    if transmission_ws:
                        transmission = mtd[transmission_ws].readY(0)[0]
                        transmission_err = mtd[transmission_ws].readE(0)[0]
                        ApplyTransmissionCorrection(InputWorkspace=ws, TransmissionValue=transmission,
                                                    TransmissionError=transmission_err, OutputWorkspace=ws)
                    SANSSolidAngleCorrection(InputWorkspace=ws, OutputWorkspace=ws)
                    # normalise by pixel solid angle, take the D(2t=0)
                    run = mtd[ws].getRun()
                    l2 = run.getLogData('L2').value
                    dx = run.getLogData('pixel_width').value
                    dy = run.getLogData('pixel_height').value
                    Scale(InputWorkspace=ws, Factor=(l2 * l2) / (dx * dy), OutputWorkspace=ws)
                    if process != 'Container':
                        container = self.getPropertyValue('ContainerInputWorkspace')
                        if container:
                            Minus(LHSWorkspace=ws, RHSWorkspace=container, OutputWorkspace=ws)
                        mask = self.getPropertyValue('MaskedInputWorkspace')
                        if mask:
                            MaskDetectors(Workspace=ws, MaskedWorkspace=mask)
                        thickness = self.getProperty('SampleThickness').value
                        NormaliseByThickness(InputWorkspace=ws, OutputWorkspace=ws, SampleThickness=thickness)
                        if process == 'Reference':
                            sensitivity_out = self.getPropertyValue('SensitivityOutputWorkspace')
                            if sensitivity_out:
                                CalculateEfficiency(InputWorkspace=ws, OutputWorkspace=sensitivity_out)
                                self.setProperty('SensitivityOutputWorkspace', mtd[sensitivity_out])
                        elif process == 'Sample':
                            reference = self.getPropertyValue('ReferenceInputWorkspace')
                            coll_ws = ''
                            if reference:
                                Divide(LHSWorkspace=ws, RHSWorkspace=reference, OutputWorkspace=ws)
                                coll_ws = reference
                            else:
                                sensitivity_in = self.getPropertyValue('SensitivityInputWorkspace')
                                if sensitivity_in:
                                    Divide(LHSWorkspace=ws, RHSWorkspace=sensitivity_in, OutputWorkspace=ws)
                                if beam:
                                    coll_ws = beam
                                    flux = mtd[beam].getRun().getLogData('BeamFluxValue').value
                                    ferr = mtd[beam].getRun().getLogData('BeamFluxError').value
                                    flux_ws = ws + '_flux'
                                    CreateSingleValuedWorkspace(DataValue=flux, ErrorValue=ferr, OutputWorkspace=flux_ws)
                                    Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws)
                                    DeleteWorkspace(flux_ws)
                            if coll_ws:
                                sample_coll = mtd[ws].getRun().getLogData('collimation.actual_position').value
                                ref_coll = mtd[coll_ws].getRun().getLogData('collimation.actual_position').value
                                flux_factor = (sample_coll ** 2) / (ref_coll ** 2)
                                self.log().notice('Flux factor is: ' + str(flux_factor))
                                Scale(InputWorkspace=ws, Factor=flux_factor, OutputWorkspace=ws)
                                ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws,
                                                     NaNValue=0., NaNError=0., InfinityValue=0., InfinityError=0.)

        CalculateQMinMax(Workspace=ws)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ILLSANSReduction)
