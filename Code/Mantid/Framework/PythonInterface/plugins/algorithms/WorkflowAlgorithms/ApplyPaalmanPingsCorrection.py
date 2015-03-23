from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty, \
                       PropertyMode, MatrixWorkspace
from mantid.kernel import Direction, logger


class ApplyPaalmanPingsCorrection(PythonAlgorithm):

    _sample_ws_name = None
    _corrections_ws_name = None
    _use_can = False
    _can_ws_name = None
    _use_corrections = False
    _can_scale_factor = 1.0
    _scale_can = False
    _output_ws_name = None
    _corrections = None
    _scaled_container = None


    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"


    def summary(self):
        return "Applies a calculated absorption correction in the Paalman and Pings factor style."


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '',
                             direction=Direction.Input),
                             doc='Name for the input Sample workspace.')

        self.declareProperty(WorkspaceGroupProperty('CorrectionsWorkspace', '',
                             optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='Name for the input Corrections workspace.')

        self.declareProperty(MatrixWorkspaceProperty('CanWorkspace', '',
                             optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='Name for the input Can workspace.')

        self.declareProperty(name='CanScaleFactor', defaultValue=1.0,
                             doc='Factor to scale the can data')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                             direction=Direction.Output),
                             doc='The output corrections workspace.')


    def PyExec(self):
        self._setup()

        if not self._use_corrections:
            logger.information('Not using corrections')
        if not self._use_can:
            logger.information('Not using container')

        # Apply container scale factor if needed
        if self._use_can:
            if self._scale_can:
                # Use temp workspace so we don't modify original data
                Scale(InputWorkspace=self._can_ws_name,
                      OutputWorkspace=self._scaled_container,
                      Factor=self._can_scale_factor,
                      Operation='Multiply')
                logger.information('Container scaled by %f' % self._can_scale_factor)
            else:
                CloneWorkspace(InputWorkspace=self._can_ws_name,
                               OutputWorkspace=self._scaled_container)

        if self._use_corrections:
            self._pre_process_corrections()

            if self._use_can:
                # Use container factors
                self._correct_sample_can()
                correction_type = 'sample_and_can_corrections'
            else:
                # Use sample factor only
                self._correct_sample()
                correction_type = 'sample_corrections_only'

        else:
            # Do simple subtraction
            self._subtract()
            correction_type = 'can_subtraction'

        # Record the container scale factor
        if self._use_can and self._scale_can:
            AddSampleLog(Workspace=self._output_ws_name,
                         LogName='apply_corr_can_scale_factor',
                         LogType='Number',
                         LogText=str(self._can_scale_factor))

        # Record the type of corrections applied
        AddSampleLog(Workspace=self._output_ws_name,
                     LogName='corrections_type',
                     LogType='String',
                     LogText=correction_type)

        self.setPropertyValue('OutputWorkspace', self._output_ws_name)

        # Remove temporary workspaces
        if self._corrections in mtd:
            DeleteWorkspace(self._corrections)
        if self._scaled_container in mtd:
            DeleteWorkspace(self._scaled_container)


    def validateInputs(self):
        """
        Validate user input.
        """

        self._setup()
        issues = dict()

        # Need something to get corrections from
        if not (self._use_can or self._use_corrections):
            error_msg = 'Must provide either CorrectionsWorkspace or CanWorkspace or both'
            issues['CorrectionsWorkspace'] = error_msg
            issues['CanWorkspace'] = error_msg

        sample_ws = mtd[self._sample_ws_name]
        if isinstance(sample_ws, MatrixWorkspace):
            sample_unit_id = sample_ws.getAxis(0).getUnit().unitID()

            # Check sample and container X axis units match
            if self._use_can:
                can_ws = mtd[self._can_ws_name]
                if isinstance(can_ws, MatrixWorkspace):
                    can_unit_id = can_ws.getAxis(0).getUnit().unitID()
                    if can_unit_id != sample_unit_id:
                        issues['CanWorkspace'] = 'X axis unit must match SampleWorkspace'
                else:
                    issues['CanWorkspace'] = 'Must be a MatrixWorkspace'
        else:
            issues['SampleWorkspace'] = 'Must be a MatrixWorkspace'

        return issues


    def _setup(self):
        """
        Get properties and setup instance variables.
        """

        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._output_ws_name = self.getPropertyValue('OutputWorkspace')

        # Get corrections workspace
        self._corrections_ws_name = self.getPropertyValue('CorrectionsWorkspace')
        self._use_corrections = self._corrections_ws_name != ''

        # Get container workspace
        self._can_ws_name = self.getPropertyValue('CanWorkspace')
        self._use_can = self._can_ws_name != ''

        self._can_scale_factor = self.getProperty('CanScaleFactor').value
        self._scale_can = self._can_scale_factor != 1.0

        # This temporary WS is needed because ConvertUnits does not like named WS in a Group
        self._corrections = '__converted_corrections'
        self._scaled_container = '__scaled_container'


    def _get_correction_factor_ws_name(self, factor_type):
        """
        Gets the full name for a correction factor workspace given the correction type.

        @param factor_type Factory type (ass, acc, acsc, assc)
        @return Full name of workspace (None if not found)
        """

        corrections_ws = mtd[self._corrections_ws_name]

        for ws_name in corrections_ws.getNames():
            if factor_type in ws_name:
                return ws_name

        return None


    def _pre_process_corrections(self):
        """
        If the sample is not in wavelength then convert the corrections to
        whatever units the sample is in.
        """

        instrument = mtd[self._sample_ws_name].getInstrument()
        unit_id = mtd[self._sample_ws_name].getAxis(0).getUnit().unitID()
        logger.information('x-unit is ' + unit_id)

        factor_types = ['ass']
        if self._use_can:
            factor_types.extend(['acc', 'acsc', 'assc'])

        for factor_type in factor_types:
            input_name = self._get_correction_factor_ws_name(factor_type)
            output_name = self._corrections + '_' + factor_type

            if unit_id != 'Wavelength':
                # Configure conversion
                if unit_id == 'dSpacing':
                    emode = 'Elastic'
                    efixed = 0.0
                elif unit_id == 'DeltaE':
                    emode = 'Indirect'
                    efixed = instrument.getNumberParameter('efixed-val')[0]
                else:
                    raise ValueError('Unit %s in sample workspace is not supported' % unit_id)

                # Do conversion
                ConvertUnits(InputWorkspace=input_name,
                             OutputWorkspace=output_name,
                             Target=unit_id,
                             EMode=emode,
                             EFixed=efixed)

            else:
                # No need to convert
                CloneWorkspace(InputWorkspace=input_name,
                               OutputWorkspace=output_name)

        # Group the temporary factor workspaces (for easy removal later)
        GroupWorkspaces(InputWorkspaces=[self._corrections + '_' + f_type for f_type in factor_types],
                        OutputWorkspace=self._corrections)


    def _subtract(self):
        """
        Do a simple container subtraction (when no corrections are given).
        """

        logger.information('Using simple container subtraction')

        Minus(LHSWorkspace=self._sample_ws_name,
              RHSWorkspace=self._scaled_container,
              OutputWorkspace=self._output_ws_name)


    def _correct_sample(self):
        """
        Correct for sample only (when no container is given).
        """

        logger.information('Correcting sample')

        # Ass
        Divide(LHSWorkspace=self._sample_ws_name,
               RHSWorkspace=self._corrections + '_ass',
               OutputWorkspace=self._output_ws_name)


    def _correct_sample_can(self):
        """
        Correct for sample and container.
        """

        logger.information('Correcting sample and container')
        corrected_can_ws = '__corrected_can'

        # Acc
        Divide(LHSWorkspace=self._scaled_container,
               RHSWorkspace=self._corrections + '_acc',
               OutputWorkspace=corrected_can_ws)

        # Acsc
        Multiply(LHSWorkspace=corrected_can_ws,
                 RHSWorkspace=self._corrections + '_acsc',
                 OutputWorkspace=corrected_can_ws)
        Minus(LHSWorkspace=self._sample_ws_name,
              RHSWorkspace=corrected_can_ws,
              OutputWorkspace=self._output_ws_name)

        # Assc
        Divide(LHSWorkspace=self._output_ws_name,
               RHSWorkspace=self._corrections + '_assc',
               OutputWorkspace=self._output_ws_name)

        DeleteWorkspace(corrected_can_ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ApplyPaalmanPingsCorrection)
