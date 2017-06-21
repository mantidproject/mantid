from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (mtd, CloneWorkspace, DeleteWorkspace, GroupWorkspaces,
                              Divide, Minus, Scale, ScaleX, AddSampleLogMultiple)
from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty,
                        PropertyMode, MatrixWorkspace, Progress)
from mantid.kernel import (Direction, logger)


class ApplyMCAbsorptionCorrection(PythonAlgorithm):
    _sample_ws_name = None

    _corrections_ws_name = None
    _use_can = False
    _can_ws_name = None
    _use_corrections = False
    _can_scale_factor = 1.0
    _scale_can = False
    _scaled_container = None
    _shift_can = False
    _shifted_container = None
    _can_shift_factor = 0.0

    _output_ws_name = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Applies a calculated Monte Carlo absorption correction."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Name for the input Sample workspace.')

        self.declareProperty(WorkspaceGroupProperty('CorrectionsWorkspace', '',
                                                    optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='Name for the input Corrections workspace.')

        self.declareProperty(MatrixWorkspaceProperty('CanWorkspace', '',
                                                     optional=PropertyMode.Optional, direction=Direction.Input),
                             doc='Name for the input Can workspace.')

        self.declareProperty(name='CanScaleFactor', defaultValue=1.0,
                             doc='Factor to scale the can data')

        self.declareProperty(name='CanShiftFactor', defaultValue=0.0,
                             doc='Amount by which to shift the container data')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output corrections workspace.')

    def PyExec(self):
        self._setup()

        prog_container = Progress(self, start=0.0, end=0.2, nreports=4)
        prog_container.report('Starting algorithm')

        if self._use_can:

            # Apply container shift if needed
            if self._shift_can:
                # Use temp workspace so we don't modify data
                prog_container.report('Shifting can')
                self._scale_x(self._can_ws_name, self._shifted_container, self._can_shift_factor)
                logger.information('Container data shifted by {}'.format(self._can_shift_factor))
            else:
                prog_container.report('Cloning Workspace')
                self._clone_ws(self._can_ws_name, self._shifted_container)

            # Apply container scale factor if needed
            if self._scale_can:
                # Use temp workspace so we don't modify original data
                prog_container.report('Scaling container')
                self._scale(self._shifted_container, self._scaled_container, self._can_scale_factor)
                logger.information('Container scaled by %f' % self._can_scale_factor)
            else:
                prog_container.report('Cloning Workspace')
                self._clone_ws(self._shifted_container, self._scaled_container)

        prog_corr = Progress(self, start=0.2, end=0.6, nreports=2)
        prog_corr.report('Preprocessing corrections')
        self._pre_process_corrections()

        if self._use_can:
            # Use container factors
            prog_corr.report('Correcting sample and can')
            self._correct_sample_can()
            correction_type = 'sample_and_can_corrections'
        else:
            # Use sample factor Ass only
            prog_corr.report('Correcting sample')
            self._divide(self._sample_ws_name, self._corrections + '_ass',
                         self._output_ws_name)
            correction_type = 'sample_corrections_only'

        sample_logs = [('corrections_type', correction_type),
                       ('sample_filename', self._sample_ws_name),
                       ('corrections_filename', self._corrections_ws_name)]

        if self._use_can:
            sample_logs.append(('container_filename', self._can_ws_name))
            if self._scale_can:
                sample_logs.append(('container_scale', self._can_scale_factor))

            if self._shift_can:
                sample_logs.append(('container_shift', self._can_shift_factor))

        log_names = [item[0] for item in sample_logs]
        log_values = [item[1] for item in sample_logs]
        self._add_sample_log_mult(self._output_ws_name, log_names, log_values)

        self.setPropertyValue('OutputWorkspace', self._output_ws_name)

        # Remove temporary workspaces
        prog_corr.report('Deleting Workspaces')
        if self._corrections in mtd:
            self._delete_ws(self._corrections)
        if self._scaled_container in mtd:
            self._delete_ws(self._scaled_container)
        if self._shifted_container in mtd:
            self._delete_ws(self._shifted_container)
        prog_corr.report('Algorithm Complete')

    def validateInputs(self):
        """
		Validate user input.
		"""

        self._setup()
        issues = dict()

        # Need something to get corrections from
        if self._corrections_ws_name == '':
            issues['CorrectionsWorkspace'] = 'Must provide CorrectionsWorkspace'

        # Check sample and container X axis units match
        if self._use_can:
            can_unit = mtd[self._can_ws_name].getAxis(0).getUnit().unitID()
            if can_unit != self._unit:
                raise ValueError('Container and Sample unit NOT the same')

        return issues

    def _setup(self):
        """
		Get properties and setup instance variables.
		"""

        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        self._unit = mtd[self._sample_ws_name].getAxis(0).getUnit().unitID()
        logger.information('Input X-unit is %s' % self._unit)

        self._output_ws_name = self.getPropertyValue('OutputWorkspace')

        # Get corrections workspace
        self._corrections_ws_name = self.getPropertyValue('CorrectionsWorkspace')
        self._use_corrections = self._corrections_ws_name != ''

        # Get container workspace
        self._can_ws_name = self.getPropertyValue('CanWorkspace')
        self._use_can = self._can_ws_name != ''
        if not self._use_can:
            logger.information('Not using container')

        self._can_scale_factor = self.getProperty('CanScaleFactor').value
        self._scale_can = self._can_scale_factor != 1.0

        self._can_shift_factor = self.getProperty('CanShiftFactor').value
        self._shift_can = self._can_shift_factor != 0.0

        # This temporary WS is needed because ConvertUnits does not like named WS in a Group
        self._corrections = '__converted_corrections'
        self._scaled_container = '__scaled_container'
        self._shifted_container = '__shifted_container'

    def _get_correction_factor_ws_name(self, factor_type):
        """
		Gets the full name for a correction factor workspace given the correction type.

		@param factor_type Factory type (ass, acc)
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

        factor_types = ['ass']
        if self._use_can:
            factor_types.append('acc')

        for factor_type in factor_types:
            input_name = self._get_correction_factor_ws_name(factor_type)
            output_name = self._corrections + '_' + factor_type

            self._clone_ws(input_name, output_name)

        corr_unit = mtd[self._corrections + '_ass'].getAxis(0).getUnit().unitID()
        if corr_unit != self._unit:
            raise ValueError('Sample and its correction unit NOT the same')

        if self._use_can:
            can_unit = mtd[self._can_ws_name].getAxis(0).getUnit().unitID()
            corr_unit = mtd[self._corrections + '_acc'].getAxis(0).getUnit().unitID()
            if corr_unit != can_unit:
                raise ValueError('Container and its correction unit NOT the same')

        # Group the temporary factor workspaces (for easy removal later)
        self._group_ws([self._corrections + '_' + f_type for f_type in factor_types],
                       self._corrections)

    def _correct_sample_can(self):
        """
		Correct for sample and container.
		"""

        corrected_can_ws = '__corrected_can'

        # Ass
        self._divide(self._sample_ws_name, self._corrections + '_ass',
                     self._output_ws_name)

        # Acc
        self._divide(self._scaled_container, self._corrections + '_acc',
                     corrected_can_ws)

        self._minus(self._output_ws_name, corrected_can_ws, self._output_ws_name)

        self._delete_ws(corrected_can_ws)

    def _clone_ws(self, input_ws, output_ws):
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone_alg.setProperty("InputWorkspace", input_ws)
        clone_alg.setProperty("OutputWorkspace", output_ws)
        clone_alg.execute()
        mtd.addOrReplace(output_ws, clone_alg.getProperty("OutputWorkspace").value)

    def _delete_ws(self, input_ws):
        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        delete_alg.setProperty("Workspace", input_ws)
        delete_alg.execute()

    def _group_ws(self, input_ws, output_ws):
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", input_ws)
        group_alg.setProperty("OutputWorkspace", output_ws)
        group_alg.execute()
        mtd.addOrReplace(output_ws, group_alg.getProperty("OutputWorkspace").value)

    def _divide(self, lhs_ws, rhs_ws, output_ws):
        divide_alg = self.createChildAlgorithm("Divide", enableLogging=False)
        divide_alg.setProperty("LHSWorkspace", lhs_ws)
        divide_alg.setProperty("RHSWorkspace", rhs_ws)
        divide_alg.setProperty("OutputWorkspace", output_ws)
        divide_alg.execute()
        mtd.addOrReplace(output_ws, divide_alg.getProperty("OutputWorkspace").value)

    def _minus(self, lhs_ws, rhs_ws, output_ws):
        minus_alg = self.createChildAlgorithm("Minus", enableLogging=True)
        minus_alg.setProperty("LHSWorkspace", lhs_ws)
        minus_alg.setProperty("RHSWorkspace", rhs_ws)
        minus_alg.setProperty("OutputWorkspace", output_ws)
        minus_alg.execute()
        mtd.addOrReplace(output_ws, minus_alg.getProperty("OutputWorkspace").value)

    def _add_sample_log_mult(self, input_ws, log_names, log_values):
        sample_log_mult_alg = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        sample_log_mult_alg.setProperty("Workspace", input_ws)
        sample_log_mult_alg.setProperty("LogNames", log_names)
        sample_log_mult_alg.setProperty("LogValues", log_values)
        sample_log_mult_alg.execute()

    def _scale(self, input_ws, output_ws, factor):
        scale_alg = self.createChildAlgorithm("Scale", enableLogging=False)
        scale_alg.setProperty("InputWorkspace", input_ws)
        scale_alg.setProperty("OutputWorkspace", output_ws)
        scale_alg.setProperty("Factor", factor)
        scale_alg.setProperty("Operation", 'Multiply')
        scale_alg.execute()
        mtd.addOrReplace(output_ws, scale_alg.getProperty("OutputWorkspace").value)

    def _scale_x(self, input_ws, output_ws, factor):
        scale_x_alg = self.createChildAlgorithm("ScaleX", enableLogging=False)
        scale_x_alg.setProperty("InputWorkspace", input_ws)
        scale_x_alg.setProperty("OutputWorkspace", output_ws)
        scale_x_alg.setProperty("Factor", factor)
        scale_x_alg.setProperty("Operation", 'Add')
        scale_x_alg.execute()
        mtd.addOrReplace(output_ws, scale_x_alg.getProperty("OutputWorkspace").value)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ApplyMCAbsorptionCorrection)
