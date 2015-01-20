# Algorithm to start Bayes programs
from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty
from mantid.kernel import Direction
from mantid import logger

import os.path
import numpy as np

class SofQWMoments(DataProcessorAlgorithm):

    def category(self):
    	return "Workflow\\MIDAS;PythonAlgorithms"

    def summary (self):
    	return "Calculates the nth moment of y(q,w)"

    def PyInit(self):
    	self.declareProperty(MatrixWorkspaceProperty("Sample", "", Direction.Input), doc="Sample to use.")
    	self.declareProperty(name='EnergyMin', defaultValue=-0.5, doc='Minimum energy for fit. Default=-0.5')
    	self.declareProperty(name='EnergyMax', defaultValue=0.5, doc='Maximum energy for fit. Default=0.5')
    	self.declareProperty(name='Scale', defaultValue=1.0, doc='Scale factor to multiply y(Q,w). Default=1.0')
    	self.declareProperty(name='Plot', defaultValue=False, doc='Switch Plot Off/On')
    	self.declareProperty(name='Save', defaultValue=False, doc='Switch Save result to nxs file Off/On')

    	self.declareProperty(WorkspaceGroupProperty("OutputWorkspace", "", Direction.Output), doc="group_workspace workspace that includes all calculated moments.")

    def PyExec(self):
    	from IndirectCommon import CheckHistZero, CheckElimits, StartTime, EndTime, getDefaultWorkingDirectory

    	sample_workspace = self.getPropertyValue('Sample')
    	output_workspace = self.getPropertyValue('OutputWorkspace')
    	factor = self.getProperty('Scale').value
    	emin = self.getProperty('EnergyMin').value
    	emax = self.getProperty('EnergyMax').value
    	erange = [emin, emax]

    	Plot = self.getProperty('Plot').value
    	Save = self.getProperty('Save').value

    	StartTime('SofQWMoments')
    	num_spectra,num_w = CheckHistZero(sample_workspace)

        logger.information('Sample %s has %d Q values & %d w values' % (sample_workspace, num_spectra, num_w))

    	x = np.asarray(mtd[sample_workspace].readX(0))
    	CheckElimits(erange,x)

    	samWS = '__temp_sqw_moments_cropped'
    	CropWorkspace(InputWorkspace=sample_workspace, OutputWorkspace=samWS, XMin=erange[0], XMax=erange[1])

        logger.information('Energy range is %f to %f' % (erange[0], erange[1]))

    	if factor > 0.0:
    	    Scale(InputWorkspace=samWS, OutputWorkspace=samWS, Factor=factor, Operation='Multiply')
            logger.information('y(q,w) scaled by %f' % factor)

    	#calculate delta x
    	ConvertToPointData(InputWorkspace=samWS, OutputWorkspace=samWS)
    	x = np.asarray(mtd[samWS].readX(0))
    	x_workspace = CreateWorkspace(OutputWorkspace="__temp_sqw_moments_x", DataX=x, DataY=x, UnitX="DeltaE")

    	#calculate moments
    	m0 = output_workspace + '_M0'
    	m1 = output_workspace + '_M1'
    	m2 = output_workspace + '_M2'
    	m3 = output_workspace + '_M3'
    	m4 = output_workspace + '_M4'

    	Multiply(x_workspace, samWS, OutputWorkspace=m1)
    	Multiply(x_workspace, m1, OutputWorkspace=m2)
    	Multiply(x_workspace, m2, OutputWorkspace=m3)
    	Multiply(x_workspace, m3, OutputWorkspace=m4)
    	DeleteWorkspace(m3)

    	ConvertToHistogram(InputWorkspace=samWS, OutputWorkspace=samWS)
    	Integration(samWS, OutputWorkspace=m0)

    	moments = [m1, m2, m4]
    	for moment_ws in moments:
    		ConvertToHistogram(InputWorkspace=moment_ws, OutputWorkspace=moment_ws)
    		Integration(moment_ws, OutputWorkspace=moment_ws)
    		Divide(moment_ws, m0, OutputWorkspace=moment_ws)

    	DeleteWorkspace(samWS)
    	DeleteWorkspace(x_workspace)

    	#create output workspace
    	extensions = ['_M0', '_M1', '_M2', '_M4']
    	for ext in extensions:
    		ws_name = output_workspace+ext
    		Transpose(InputWorkspace=ws_name, OutputWorkspace=ws_name)
    		ConvertToHistogram(InputWorkspace=ws_name, OutputWorkspace=ws_name)
    		ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, Target='MomentumTransfer', EMode='Indirect')

    		CopyLogs(InputWorkspace=sample_workspace, OutputWorkspace=ws_name)
    		AddSampleLog(Workspace=ws_name, LogName="energy_min", LogType="Number", LogText=str(emin))
    		AddSampleLog(Workspace=ws_name, LogName="energy_max", LogType="Number", LogText=str(emax))
    		AddSampleLog(Workspace=ws_name, LogName="scale_factor", LogType="Number", LogText=str(factor))

    	# Group output workspace
    	group_workspaces = ','.join([output_workspace+ext for ext in extensions])
    	GroupWorkspaces(InputWorkspaces=group_workspaces, OutputWorkspace=output_workspace)

    	if Save:
    		workdir = getDefaultWorkingDirectory()
    		opath = os.path.join(workdir,output_workspace+'.nxs')
    		SaveNexusProcessed(InputWorkspace=output_workspace, Filename=opath)
            logger.information('Output file : ' + opath)

    	if Plot:
    	    self._plot_moments(output_workspace)

    	self.setProperty("OutputWorkspace", output_workspace)

    	EndTime('SofQWMoments')

    def _plot_moments(self, inputWS):
    	from IndirectImport import import_mantidplot
    	mp = import_mantidplot()

    	mp.plotSpectrum(inputWS+'_M0', 0)
    	mp.plotSpectrum([inputWS+'_M2', inputWS+'_M4'], 0)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(SofQWMoments)
