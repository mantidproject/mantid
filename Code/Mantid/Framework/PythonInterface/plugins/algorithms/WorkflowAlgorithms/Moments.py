"""*WIKI*

Calculates the <math>n^{th}</math> moment <math>M_n</math> of <math>y(Q,w)</math> where <math>M_n</math> is the integral of <math>w^n*y(Q,w)</math> over all w for <math>n=0</math> to 4.

*WIKI*"""
# Algorithm to start Bayes programs
from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty
from mantid.kernel import Direction
from mantid import logger

import os.path
import numpy as np

class Moments(PythonAlgorithm):

	def category(self):
		return "Workflow\\MIDAS;PythonAlgorithms"

	def PyInit(self):
		self.setOptionalMessage("Calculates the nth moment of y(q,w)")
		self.setWikiSummary("Calculates the nth moment of y(q,w)")

		self.declareProperty(MatrixWorkspaceProperty("Sample", "", Direction.Input), doc="Sample to use.")
		self.declareProperty(name='EnergyMin', defaultValue=-0.5, doc='Minimum energy for fit. Default=-0.5')
		self.declareProperty(name='EnergyMax', defaultValue=0.5, doc='Maximum energy for fit. Default=0.5')
		self.declareProperty(name='Scale', defaultValue=1.0, doc='Scale factor to multiply y(Q,w). Default=1.0')
		self.declareProperty(name='Verbose', defaultValue=False, doc='Switch Verbose Off/On')
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

		Verbose = self.getProperty('Verbose').value
		Plot = self.getProperty('Plot').value
		Save = self.getProperty('Save').value

		StartTime('Moments')
		num_spectra,num_w = CheckHistZero(sample_workspace)

		if Verbose:
			text = 'Sample %s has %d Q values & %d w values' % (sample_workspace, num_spectra, num_w)
			logger.notice(text)

		x = np.asarray(mtd[sample_workspace].readX(0))
		CheckElimits(erange,x)

		samWS = '__moments_cropped_ws'
		CropWorkspace(InputWorkspace=sample_workspace, OutputWorkspace=samWS, XMin=erange[0], XMax=erange[1])

		if Verbose:
		    logger.notice('Energy range is %f to %f' % (erange[0], erange[1]))

		if factor > 0.0:
		    Scale(InputWorkspace=samWS, OutputWorkspace=samWS, Factor=factor, Operation='Multiply')
		    if Verbose:
		        logger.notice('y(q,w) scaled by %f' % factor)

		#calculate delta x
		x = np.asarray(mtd[samWS].readX(0))
		delta_x = x[1:] - x[:-1]
		x = x[:-1]
		#calculate moments for workspace
		yM0, yM1, yM2, yM3, yM4 = [],[],[],[],[]
		for index in range(num_spectra):
		    if Verbose:
		        logger.notice('group_workspace %d at Q = %d' % (index+1, index))

		    y = np.asarray(mtd[samWS].readY(index))

		    S0 = y * delta_x
		    m0 = np.sum(S0)

		    S1 = (x * S0)
		    m1 = np.sum(S1) / m0
		    S2 = x * S1
		    m2 = np.sum(S2) / m0
		    S3 = x * S2
		    m3 = np.sum(S3) / m0
		    S4 = x * S3
		    m4 = np.sum(S4) / m0

		    if Verbose:
			    text = 'M0 = %f ; M2 = %f ; M4 = %f' % (m0, m2, m4)
			    logger.notice(text)

		    yM0.append(m0)
		    yM1.append(m1)
		    yM2.append(m2)
		    yM4.append(m4)

		output_workspace = output_workspace + '_Moments'

		#create output workspace
		Q = np.arange(num_spectra)
		extensions = ['_M0', '_M1', '_M2', '_M4']
		y_data = [yM0, yM1, yM2, yM4]

		for ext, data in zip(extensions, y_data):
			CreateWorkspace(OutputWorkspace=output_workspace+ext, DataX=Q, DataY=data,Nspec=1, UnitX='MomentumTransfer')
			CopyLogs(InputWorkspace=sample_workspace, OutputWorkspace=output_workspace+ext)
			AddSampleLog(Workspace=output_workspace+ext, LogName="energy_min", LogType="Number", LogText=str(emin))
			AddSampleLog(Workspace=output_workspace+ext, LogName="energy_max", LogType="Number", LogText=str(emax))
			AddSampleLog(Workspace=output_workspace+ext, LogName="scale_factor", LogType="Number", LogText=str(factor))

		#group ouput workspace
		group_workspaces = ','.join([output_workspace+ext for ext in extensions])
		GroupWorkspaces(InputWorkspaces=group_workspaces,OutputWorkspace=output_workspace)
		DeleteWorkspace(samWS)

		if Save:
			workdir = getDefaultWorkingDirectory()
			opath = os.path.join(workdir,output_workspace+'.nxs')
			SaveNexusProcessed(InputWorkspace=output_workspace, Filename=opath)

			if Verbose:
				logger.notice('Output file : ' + opath)

		if Plot:
		    self._plot_moments(output_workspace)

		self.setProperty("OutputWorkspace", output_workspace)

		EndTime('Moments')

	def _plot_moments(self, inputWS):
		from IndirectImport import import_mantidplot
		mp = import_mantidplot()

		mp.plotSpectrum(inputWS+'_M0',0)
		mp.plotSpectrum([inputWS+'_M2',inputWS+'_M4'],0)

AlgorithmFactory.subscribe(Moments)         # Register algorithm with Mantid
