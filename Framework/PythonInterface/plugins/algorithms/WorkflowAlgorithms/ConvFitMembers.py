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
        self._ws_names = []

    def PyExec(self):
        self._setup()

        axis = self._result_ws[0].getAxis(1)
        paras = axis.extractValues()
        run = self._result_ws[0].getRun()

        # Check whether delta function and/or lorentzian members
        # were used in the convolution fitting. Log how many of each
        # and if used, add them to the member list.
        log_name = 'delta_function'
        delta = run[log_name].value
        log_name = 'lorentzians'
        lorentzians = run[log_name].value
        logger.information('Lorentzians = %i ; Delta = %s' % (lorentzians, delta))
        members = []
        numb_members = 4
        for i in range(numb_members):
            members.append(paras[i])
        if delta == 'true':
            members.append('Delta')
            numb_members += 1
        for i in range(lorentzians):
            members.append('Lorentzian' + str(i + 1))
            numb_members += 1
        logger.information('Members : %s' % (members))
        if numb_members != len(paras):
            raise ValueError('Number of members incorrect')

        for idx, ws in enumerate(self._result_ws):

            # Extract each of the convolution members from the current
            # workspace and add them to an output workspace.
            self._extract_members(ws, members, idx==0)

        ax = NumericAxis.create(idx+1)
        for i in range(idx+1):
            ax.setValue(i, self._qvalues[i])
        for ws in self._ws_names:
            mtd[ws].replaceAxis(1, ax)
            mtd[ws].setYUnitLabel("MomentumTransfer")
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)
        group_alg.setProperty("InputWorkspaces", self._ws_names)
        group_alg.setProperty("OutputWorkspace", self._output_ws)
        group_alg.execute()
        mtd.addOrReplace(self._output_ws, group_alg.getProperty("OutputWorkspace").value)

    def _extract_members(self, input_ws, members, create_output_ws):
        """
        Extracts the convolution fit members from the input workspace.
        If create_output_ws is True, a new workspace is created for each
        of these members. Else, the values for these members are appended
        to an existing working output workspace - as defined by this object.

        :param input_ws:            The input workspace.
        :param members:             The members to extract.
        :param create_output_ws:    Whether to create a new output workspace.
        """

        # Iterate through all specified members.
        for idx, member in enumerate(members):

            # Extract the current member into a temporary workspace.
            output_ws = self._output_ws + '_' + member
            extract_alg = self.createChildAlgorithm("ExtractSpectra", enableLogging=False)
            extract_alg.setProperty("InputWorkspace", input_ws)
            extract_alg.setProperty("OutputWorkspace", '__temp')
            extract_alg.setProperty("StartWorkspaceIndex", idx)
            extract_alg.setProperty("EndWorkspaceIndex", idx)
            extract_alg.execute()

            # Check whether to create a new workspace.
            if create_output_ws:
                self._ws_names.append(output_ws)
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


AlgorithmFactory.subscribe(ConvFitMembers)