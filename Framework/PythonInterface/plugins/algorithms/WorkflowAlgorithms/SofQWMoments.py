#pylint: disable=no-init,invalid-name
# Algorithm to start Bayes programs
from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, \
                       WorkspaceGroupProperty, Progress
from mantid.kernel import Direction
from mantid import logger

import numpy as np

class SofQWMoments(DataProcessorAlgorithm):

    def category(self):
        return "Workflow\\MIDAS"


    def summary (self):
        return "Calculates the nth moment of y(q,w)"


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("Sample", "", Direction.Input),
                             doc="Sample to use.")
        self.declareProperty(name='EnergyMin', defaultValue=-0.5,
                             doc='Minimum energy for fit. Default=-0.5')
        self.declareProperty(name='EnergyMax', defaultValue=0.5,
                             doc='Maximum energy for fit. Default=0.5')
        self.declareProperty(name='Scale', defaultValue=1.0,
                             doc='Scale factor to multiply y(Q,w). Default=1.0')
        self.declareProperty(WorkspaceGroupProperty("OutputWorkspace", "", Direction.Output),
                             doc="group_workspace workspace that includes all calculated moments.")

    #pylint: disable=too-many-locals
    def PyExec(self):
        from IndirectCommon import CheckElimits

        workflow_prog = Progress(self, start=0.0, end=1.0, nreports=20)
        workflow_prog.report('Setting up algorithm')
        sample_workspace = self.getPropertyValue('Sample')
        output_workspace = self.getPropertyValue('OutputWorkspace')
        factor = self.getProperty('Scale').value
        emin = self.getProperty('EnergyMin').value
        emax = self.getProperty('EnergyMax').value
        erange = [emin, emax]
        workflow_prog.report('Validating input')
        num_spectra,num_w = self.CheckHistZero(sample_workspace)

        logger.information('Sample %s has %d Q values & %d w values' % (sample_workspace, num_spectra, num_w))

        x_data = np.asarray(mtd[sample_workspace].readX(0))
        CheckElimits(erange,x_data)

        workflow_prog.report('Cropping Workspace')
        samWS = '__temp_sqw_moments_cropped'
        CropWorkspace(InputWorkspace=sample_workspace, OutputWorkspace=samWS,
                      XMin=erange[0], XMax=erange[1])

        logger.information('Energy range is %f to %f' % (erange[0], erange[1]))

        if factor > 0.0:
            workflow_prog.report('Scaling Workspace by factor %f' % factor)
            Scale(InputWorkspace=samWS, OutputWorkspace=samWS, Factor=factor, Operation='Multiply')
            logger.information('y(q,w) scaled by %f' % factor)

        #calculate delta x
        workflow_prog.report('Converting to point data')
        ConvertToPointData(InputWorkspace=samWS, OutputWorkspace=samWS)
        x_data = np.asarray(mtd[samWS].readX(0))
        workflow_prog.report('Creating temporary data workspace')
        x_workspace = CreateWorkspace(OutputWorkspace="__temp_sqw_moments_x",
                                      DataX=x_data, DataY=x_data, UnitX="DeltaE")

        #calculate moments
        moments_0 = output_workspace + '_M0'
        moments_1 = output_workspace + '_M1'
        moments_2 = output_workspace + '_M2'
        moments_3 = output_workspace + '_M3'
        moments_4 = output_workspace + '_M4'

        workflow_prog.report('Multiplying Workspaces by moments')
        Multiply(x_workspace, samWS, OutputWorkspace=moments_1)
        Multiply(x_workspace, moments_1, OutputWorkspace=moments_2)
        Multiply(x_workspace, moments_2, OutputWorkspace=moments_3)
        Multiply(x_workspace, moments_3, OutputWorkspace=moments_4)
        DeleteWorkspace(moments_3)

        workflow_prog.report('Converting to Histogram')
        ConvertToHistogram(InputWorkspace=samWS, OutputWorkspace=samWS)
        workflow_prog.report('Integrating result')
        Integration(samWS, OutputWorkspace=moments_0)

        moments = [moments_1, moments_2, moments_4]
        for moment_ws in moments:
            workflow_prog.report('Processing workspace %s' % moment_ws)
            ConvertToHistogram(InputWorkspace=moment_ws, OutputWorkspace=moment_ws)
            Integration(moment_ws, OutputWorkspace=moment_ws)
            Divide(moment_ws, moments_0, OutputWorkspace=moment_ws)

        workflow_prog.report('Deleting Workspaces')
        DeleteWorkspace(samWS)
        DeleteWorkspace(x_workspace)

        #create output workspace
        extensions = ['_M0', '_M1', '_M2', '_M4']
        for ext in extensions:
            ws_name = output_workspace+ext
            workflow_prog.report('Processing Workspace %s' % ext)
            Transpose(InputWorkspace=ws_name, OutputWorkspace=ws_name)
            ConvertToHistogram(InputWorkspace=ws_name, OutputWorkspace=ws_name)
            ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name,
                         Target='MomentumTransfer', EMode='Indirect')

            CopyLogs(InputWorkspace=sample_workspace, OutputWorkspace=ws_name)
            workflow_prog.report('Adding Sample logs to %s' % ws_name)
            AddSampleLog(Workspace=ws_name, LogName="energy_min",
                         LogType="Number", LogText=str(emin))
            AddSampleLog(Workspace=ws_name, LogName="energy_max",
                         LogType="Number", LogText=str(emax))
            AddSampleLog(Workspace=ws_name, LogName="scale_factor",
                         LogType="Number", LogText=str(factor))

        # Group output workspace
        workflow_prog.report('Grouping OutputWorkspace')
        group_workspaces = ','.join([output_workspace+ext for ext in extensions])
        GroupWorkspaces(InputWorkspaces=group_workspaces, OutputWorkspace=output_workspace)

        self.setProperty("OutputWorkspace", output_workspace)
        workflow_prog.report('Algorithm complete')

    def CheckHistZero(self, inWS):
        """
        Retrieves basic info on a workspace
        Checks the workspace is not empty, then returns the number of histogram and
        the number of X-points, which is the number of bin boundaries minus one
        Args:
          @param inWS  2D workspace
        Returns:
          @return num_hist - number of histograms in the workspace
          @return ntc - number of X-points in the first histogram, which is the number of bin
               boundaries minus one. It is assumed all histograms have the same
               number of X-points.
        Raises:
          @exception ValueError - Workspace has no histograms
        """
        num_hist = s_api.mtd[inWS].getNumberHistograms()  # no. of hist/groups in WS
        if num_hist == 0:
            raise ValueError('Workspace ' + inWS + ' has NO histograms')
        x_in = s_api.mtd[inWS].readX(0)
        ntc = len(x_in) - 1  # no. points from length of x array
        if ntc == 0:
            raise ValueError('Workspace ' + inWS + ' has NO points')
        return num_hist, ntc

# Register algorithm with Mantid
AlgorithmFactory.subscribe(SofQWMoments)
