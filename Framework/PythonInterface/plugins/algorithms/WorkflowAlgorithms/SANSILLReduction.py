from __future__ import (absolute_import, division, print_function)

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, LogicOperator, PropertyCriterion, StringListValidator
from mantid.simpleapi import *
from math import fabs


class SANSILLReduction(PythonAlgorithm):

    def category(self):
        return "ILL\\SANS"

    def summary(self):
        return 'Performs SANS data reduction at the ILL.'

    def seeAlso(self):
        return []

    def name(self):
        return "SANSILLReduction"

    def validateInputs(self):
        issues = dict()
        if 'ProcessAs' == 'Transmission' and self.getProperty('BeamInputWorkspace').isDefault:
            issues['BeamInputWorkspace'] = 'Beam input workspace is mandatory for transmission calculation.'

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        options = ['Absorber', 'Beam', 'Transmission', 'Container', 'Reference', 'Sample']

        self.declareProperty(name='ProcessAs',
                             defaultValue='Sample',
                             validator=StringListValidator(options),
                             doc='Choose the process type.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='The output workspace based on the value of ProcessAs.')

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

        self.declareProperty('BeamRadius', 0.05, validator=FloatBoundedValidator(lower=0.),
                             doc='Beam raduis [m]; used for beam center finding and transmission calculations.')

        self.setPropertySettings('BeamRadius',
                                 EnabledWhenProperty(beam, transmission, LogicOperator.Or))

        self.declareProperty('BeamFinderMethod', 'DirectBeam', StringListValidator(['DirectBeam', 'ScatteredBeam']),
                             doc='Use direct beam method of the beam finding, if not, use the scattered beam method.')

        self.setPropertySettings('BeamFinderMethod', beam)

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
            @param radius : the radius of the cylinder [m]
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
            monID = 100000
            if mtd[ws].getInstrument().getName() == 'D33':
                monID = 500000
            ExtractSpectra(InputWorkspace=ws, DetectorList=monID, OutputWorkspace=mon)
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
        method = self.getPropertyValue('BeamFinderMethod')
        radius = self.getProperty('BeamRadius').value
        FindCenterOfMassPosition(InputWorkspace=ws, DirectBeam=(method == 'DirectBeam'), BeamRadius=radius, Output=centers)
        beam_x = mtd[centers].cell(0,1)
        beam_y = mtd[centers].cell(1,1)
        AddSampleLog(Workspace=ws, LogName='BeamCenterX', LogText=str(beam_x), LogType='Number')
        AddSampleLog(Workspace=ws, LogName='BeamCenterY', LogText=str(beam_y), LogType='Number')
        DeleteWorkspace(centers)
        MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')
        integral = self._integrate_in_radius(ws, radius)
        run = mtd[ws].getRun()
        if run.hasProperty('attenuator.attenuation_coefficient'):
            att_coeff = run.getLogData('attenuator.attenuation_coefficient').value
        elif run.hasProperty('attenuator.attenuation_value'):
            att_coeff = run.getLogData('attenuator.attenuation_value').value
        else:
            raise RuntimeError('Unable to process as beam: could not find attenuation coefficient nor value.')
        self.log().information('Found attenuator coefficient/value: {0}'.format(att_coeff))
        Scale(InputWorkspace=integral, Factor=att_coeff, OutputWorkspace=integral)
        AddSampleLog(Workspace=ws, LogName='BeamFluxValue', LogText=str(mtd[integral].readY(0)[0]), LogType='Number')
        AddSampleLog(Workspace=ws, LogName='BeamFluxError', LogText=str(mtd[integral].readE(0)[0]), LogType='Number')
        DeleteWorkspace(integral)

    @staticmethod
    def _check_distances_match(ws1, ws2):
        """
            Checks if the detector distance between two workspaces are close enough
            @param ws1 : workspace 1
            @param ws2 : workspace 2
            @return true if the detector distance difference is less than 1 cm
        """
        tolerance = 0.01 #m
        l2_1 = ws1.getRun().getLogData('L2').value
        l2_2 = ws2.getRun().getLogData('L2').value
        return fabs(l2_1 - l2_2) < tolerance

    def PyExec(self): # noqa: C901

        process = self.getPropertyValue('ProcessAs')
        ws = '__' + self.getPropertyValue('OutputWorkspace')
        LoadAndMerge(Filename=self.getPropertyValue('Run').replace(',','+'), LoaderName='LoadILLSANS', OutputWorkspace=ws)
        self._normalise(ws)
        ExtractMonitors(InputWorkspace=ws, DetectorWorkspace=ws)
        if process in ['Beam', 'Transmission', 'Container', 'Reference', 'Sample']:
            absorber_ws = self.getProperty('AbsorberInputWorkspace').value
            if absorber_ws:
                Minus(LHSWorkspace=ws, RHSWorkspace=absorber_ws, OutputWorkspace=ws)
            if process == 'Beam':
                self._process_beam(ws)
            else:
                beam_ws = self.getProperty('BeamInputWorkspace').value
                if beam_ws:
                    beam_x = beam_ws.getRun().getLogData('BeamCenterX').value
                    beam_y = beam_ws.getRun().getLogData('BeamCenterY').value
                    MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName='detector')
                    if not self._check_distances_match(mtd[ws], beam_ws):
                        self.log().warning('Different detector distances found for empty beam and sample runs!')
                if process == 'Transmission':
                    if not self._check_distances_match(mtd[ws], beam_ws):
                        self.log().warning('Different detector distances found for empty beam and transmission runs!')
                    RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=beam_ws, OutputWorkspace=ws)
                    radius = self.getProperty('BeamRadius').value
                    shapeXML = self._cylinder(radius)
                    det_list = FindDetectorsInShape(Workspace=ws, ShapeXML=shapeXML)
                    CalculateTransmission(SampleRunWorkspace=ws, DirectRunWorkspace=beam_ws,
                                          TransmissionROI=det_list, OutputWorkspace=ws)
                else:
                    transmission_ws = self.getProperty('TransmissionInputWorkspace').value
                    if transmission_ws:
                        transmission = transmission_ws.readY(0)[0]
                        transmission_err = transmission_ws.readE(0)[0]
                        ApplyTransmissionCorrection(InputWorkspace=ws, TransmissionValue=transmission,
                                                    TransmissionError=transmission_err, OutputWorkspace=ws)
                    solid_angle = ws + '_sa'
                    SolidAngle(InputWorkspace=ws, OutputWorkspace=solid_angle)
                    Divide(LHSWorkspace=ws, RHSWorkspace=solid_angle, OutputWorkspace=ws)
                    DeleteWorkspace(solid_angle)
                    if process in ['Reference', 'Sample']:
                        container_ws = self.getProperty('ContainerInputWorkspace').value
                        if container_ws:
                            if not self._check_distances_match(mtd[ws], container_ws):
                                self.log().warning(
                                    'Different detector distances found for container and sample runs!')
                            Minus(LHSWorkspace=ws, RHSWorkspace=container_ws, OutputWorkspace=ws)
                        mask_ws = self.getProperty('MaskedInputWorkspace').value
                        if mask_ws:
                            masked_ws = ws + '_mask'
                            CloneWorkspace(InputWorkspace=mask_ws, OutputWorkspace=masked_ws)
                            ExtractMonitors(InputWorkspace=masked_ws, DetectorWorkspace=masked_ws)
                            MaskDetectors(Workspace=ws, MaskedWorkspace=masked_ws)
                            DeleteWorkspace(masked_ws)
                        thickness = self.getProperty('SampleThickness').value
                        NormaliseByThickness(InputWorkspace=ws, OutputWorkspace=ws, SampleThickness=thickness)
                        if process == 'Reference':
                            sensitivity_out = self.getPropertyValue('SensitivityOutputWorkspace')
                            if sensitivity_out:
                                CalculateEfficiency(InputWorkspace=ws, OutputWorkspace=sensitivity_out)
                                mtd[sensitivity_out].getRun().addProperty('ProcessedAs', 'Sensitivity', True)
                                self.setProperty('SensitivityOutputWorkspace', mtd[sensitivity_out])
                        elif process == 'Sample':
                            reference_ws = self.getProperty('ReferenceInputWorkspace').value
                            coll_ws = None
                            if reference_ws:
                                Divide(LHSWorkspace=ws, RHSWorkspace=reference_ws, OutputWorkspace=ws)
                                coll_ws = reference_ws
                            else:
                                sensitivity_in = self.getProperty('SensitivityInputWorkspace').value
                                if sensitivity_in:
                                    Divide(LHSWorkspace=ws, RHSWorkspace=sensitivity_in, OutputWorkspace=ws)
                                if beam_ws:
                                    coll_ws = beam_ws
                                    flux = beam_ws.getRun().getLogData('BeamFluxValue').value
                                    ferr = beam_ws.getRun().getLogData('BeamFluxError').value
                                    flux_ws = ws + '_flux'
                                    CreateSingleValuedWorkspace(DataValue=flux, ErrorValue=ferr, OutputWorkspace=flux_ws)
                                    Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws)
                                    DeleteWorkspace(flux_ws)
                            if coll_ws:
                                if not self._check_distances_match(mtd[ws], coll_ws):
                                    self.log().warning(
                                        'Different detector distances found for the reference/flux and sample runs!')
                                sample_coll = mtd[ws].getRun().getLogData('collimation.actual_position').value
                                ref_coll = coll_ws.getRun().getLogData('collimation.actual_position').value
                                flux_factor = (sample_coll ** 2) / (ref_coll ** 2)
                                self.log().notice('Flux factor is: ' + str(flux_factor))
                                Scale(InputWorkspace=ws, Factor=flux_factor, OutputWorkspace=ws)
                                ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws,
                                                     NaNValue=0., NaNError=0., InfinityValue=0., InfinityError=0.)

        CalculateDynamicRange(Workspace=ws)
        mtd[ws].getRun().addProperty('ProcessedAs', process, True)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty('OutputWorkspace', mtd[ws[2:]])

# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSILLReduction)
