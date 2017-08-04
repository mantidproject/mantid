from mantid.simpleapi import (mtd, CloneWorkspace, ExtractSpectra, AppendSpectra, GetQsInQENSData)
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, PropertyMode, NumericAxis,
                        MatrixWorkspaceProperty, WorkspaceGroupProperty, Progress)
from mantid.kernel import (StringListValidator, StringMandatoryValidator, IntBoundedValidator,
                           FloatBoundedValidator, Direction, logger, CompositeValidator)

class ConvFitMembers(DataProcessorAlgorithm):
    _sample_ws = None
    _result_ws = None
    _output_ws = None
    _qvalues = None

    def category(self):
        return "Workflow\\MIDAS"

    def summary(self):
        return "Converts a ConvFit fitted parameter Table."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('SampleWorkspace', '', direction=Direction.Input),
                             doc='Input Sample Workspace (*_red)')

        self.declareProperty(WorkspaceGroupProperty('ResultWorkspace', '', direction=Direction.Input),
                             doc='Input results Group Workspace (*_Workspaces)')

        self.declareProperty(name='OutputWorkspace', defaultValue='Output',
                             doc='Output name Members workspace')

    def _setup(self):
        self._sample_ws = self.getProperty('SampleWorkspace').value
        self._result_ws = self.getProperty('ResultWorkspace').value
        self._output_ws = self.getProperty('OutputWorkspace').value
        self._qvalues = GetQsInQENSData(InputWorkspace=self._sample_ws,
                                        EnableLogging=False)

    def PyExec(self):
        self._setup()

        ws_names = []
        for idx, ws in enumerate(self._result_ws):

            if idx == 0:
                axis = ws.getAxis(1)
                paras = axis.extractValues()
                numb_paras = len(paras)
                run = ws.getRun()
                log_name = 'delta_function'
                delta = run[log_name].value
                log_name = 'lorentzians'
                lorentzians = run[log_name].value
                logger.information('Lorentzians = %i ; Delta = %s' % (lorentzians, delta))
                members = []
                for i in range(4):
                    members.append(paras[i])
                numb_members = 4
                if delta == 'true':
                    members.append('Delta')
                    numb_members += 1
                for i in range(lorentzians):
                    members.append('Lorentzian' + str(i+1))
                    numb_members += 1
                logger.information('Members : %s' % (members))
                if numb_members != numb_paras:
                    raise ValueError('Number of members incorrect')

            for ndx in range(numb_paras):
                    output_ws = self._output_ws + '_' + members[ndx]
                    extract_alg = self.createChildAlgorithm("ExtractSpectra", enableLogging=False)
                    extract_alg.setProperty("InputWorkspace", ws)
                    extract_alg.setProperty("OutputWorkspace", '__temp')
                    extract_alg.setProperty("StartWorkspaceIndex", ndx)
                    extract_alg.setProperty("EndWorkspaceIndex", ndx)
                    extract_alg.execute()
                    if idx == 0:
                        ws_names.append(output_ws)
                        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
                        clone_alg.setProperty("InputWorkspace", extract_alg.getProperty("OutputWorkspace").value)
                        clone_alg.setProperty("OutputWorkspace", output_ws)
                        clone_alg.execute()
                        mtd.addOrReplace(output_ws, clone_alg.getProperty("OutputWorkspace").value)
                    else:
                        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=False)
                        append_alg.setProperty("InputWorkspace1", output_ws)
                        append_alg.setProperty("InputWorkspace2", extract_alg.getProperty("OutputWorkspace").value)
                        append_alg.setProperty("OutputWorkspace", output_ws)
                        append_alg.execute()
                        mtd.addOrReplace(output_ws, append_alg.getProperty("OutputWorkspace").value)

        ax = NumericAxis.create(idx+1)
        for i in range(idx+1):
            ax.setValue(i, self._qvalues[i])
        for ws in ws_names:
            mtd[ws].replaceAxis(1, ax)
            mtd[ws].setYUnitLabel("MomentumTransfer")
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", ws_names)
        group_alg.setProperty("OutputWorkspace", self._output_ws)
        group_alg.execute()
        mtd.addOrReplace(self._output_ws, group_alg.getProperty("OutputWorkspace").value)

AlgorithmFactory.subscribe(ConvFitMembers)
