from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty, \
                       FileProperty, FileAction, PropertyMode
from mantid.kernel import StringListValidator, StringMandatoryValidator, Direction, logger
from mantid import config
import math, os.path, numpy as np

class ApplyPaalmanPingsCorrection(PythonAlgorithm):

    _sample_ws_name = None
    _corrections_ws_name = None
    _usecan = False
    _can_ws_name = None
    _can_corrections = False
    _scale_can = False
    _scale_factor = 1.0
    _output_ws_name = None

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc="Name for the input Sample workspace.")
        self.declareProperty(WorkspaceGroupProperty('CorrectionsWorkspace', '', optional=PropertyMode.Optional,
                             direction=Direction.Input),doc="Name for the input Corrections workspace.")

        self.declareProperty(MatrixWorkspaceProperty('CanWorkspace', '', optional=PropertyMode.Optional,
                             direction=Direction.Input),doc="Name for the input Can workspace.")
        self.declareProperty(name='CanScaleFactor', defaultValue='', doc = 'Factor to scale the can data')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output corrections workspace.')

    def PyExec(self):

        workdir = config['defaultsave.directory']
        self._setup()
        instrument = mtd[self._sample_ws_name].getInstrument()
        axis = mtd[self._sample_ws_name].getAxis(0)
        unit = axis.getUnit()
        logger.information('x-unit is ' + unit.unitID())
#       this temporary WS is needed because ConvertUnits does not like named WS in a Group
        self._corrections = '__converted_corrections'

        if self._usecan:
            if unit.unitID() != 'Wavelength':
                if unit.unitID() == 'dSpacing':
                    self._emode = 'Elastic'
                    target = 'dSpacing'
                    self._efixed = 0.0
                elif unit.unitID() == 'DeltaE':
                    self._emode = 'Indirect'
                    target = 'DeltaE'
                    self._efixed = instrument.getNumberParameter("efixed-val")[0]
                else:
                    raise ValueError('Unit ' + unit.unitID() + ' is not supported')
                ConvertUnits(InputWorkspace=self._corrections_ws_name, OutputWorkspace=self._corrections,
			                 Target=target, EMode=self._emode, EFixed=self._efixed)
            else:
                CloneWorkspace(InputWorkspace=self._corrections_ws_name, OutputWorkspace=self._corrections)

            self._scaled_container = "__scaled_container"
            if self.scale_can:
            # Use temp workspace so we don't modify original data
                Scale(InputWorkspace=self._can_ws_name, OutputWorkspace=self._scaled_container,
                      Factor=factor, Operation='Multiply')
                logger.information('Container scaled by %f' % factor)

            else:
                CloneWorkspace(InputWorkspace=self._can_ws_name, OutputWorkspace=self._scaled_container)

            if self._can_corrections:
                self._correctSampleCan()
            else:
                self._subtract()

        else:
            self._correctSample()
        self.setPropertyValue('OutputWorkspace', self._output_ws_name)

    def _setup(self):
        self._sample_ws_name = self.getPropertyValue('SampleWorkspace')
        logger.information('Sample is ' + self._sample_ws_name)
        self._corrections_ws_name = self.getPropertyValue('CorrectionsWorkspace')
        if self._corrections_ws_name == '':
            self._can_corrections = False
            logger.information('NO corrections')
        else:
            self._can_corrections = True
            logger.information('Corrections is ' + self._corrections_ws_name)
        self._can_ws_name = self.getPropertyValue('CanWorkspace')
        if self._can_ws_name == '':
            self._usecan = False
            logger.information('NO can')
        else:
            self._usecan = True
            logger.information('Can is ' + self._can_ws_name)
        if self._usecan:
            scale_factor = float(self.getProperty('CanScaleFactor').value)
            if scale_factor == 1.0:
                self.scale_can = False
            else:
                self.scale_can = True

        if self._usecan == False and self._can_corrections == False:
            raise ValueError('Nothing to do!')

        self._output_ws_name = self.getPropertyValue('OutputWorkspace')

    def _subtract(self):
        Minus(LHSWorkspace=self._sample_ws_name, RHSWorkspace=self._scaled_container,
              OutputWorkspace=self._output_ws_name)

    def _correctSample(self):
#       Ass is group 1
        Divide(LHSWorkspace=self._sample_ws_name, RHSWorkspace=self._corrections+'_1',
              OutputWorkspace=self._output_ws_name)

    def _correctSampleCan(self):
        CorrectedCanWS = '__corrected_can'
#       Acc	is group 4
        Divide(LHSWorkspace=self._scaled_container, RHSWorkspace=self._corrections+'_4',
              OutputWorkspace=CorrectedCanWS)
#       Acsc is group 3
        Multiply(LHSWorkspace=CorrectedCanWS, RHSWorkspace=self._corrections+'_3',
              OutputWorkspace=CorrectedCanWS)
        Minus(LHSWorkspace=self._sample_ws_name, RHSWorkspace=CorrectedCanWS,
              OutputWorkspace=self._output_ws_name)
#       Assc is group 2
        Divide(LHSWorkspace=self._output_ws_name, RHSWorkspace=self._corrections+'_2',
              OutputWorkspace=self._output_ws_name)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(ApplyPaalmanPingsCorrection)
#
