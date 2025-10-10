# Set of routines to normalise WISH data- new look Mantid with mantidsimple removed
from mantid.simpleapi import *
import numpy as np
from mantid.dataobjects import EventWorkspace
from itertools import cycle

class WishPowder:
    def __init__(self, cycle, calibration_dir, user_dir, sample_env="candlestick"):
        self.cycle = cycle
		self.calibration_dir = calibration_dir
		self.user_dir = user_dir
		# self.van_shape
		# self.sample_shape

	def load(self, runno, ext="raw"):
		# apply claibration?
		# what about multiple runs being summed
		wsname = f"WISH{runno:08d}"
		ws = Load(Filename=f"{wsname}.{ext}", OutputWorkspace=wsname, LoadMonitors=True)
		if isinstance(ws, EventWorkspace):
			Rebin(InputWorkspace=wsname, OutputWorkspace=wsname, Params='6000,-0.00063,110000')
			ConvertToMatrixWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname)
		# crop prompt pulse
		CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=6000, XMax=99000)
		# crop in workspace
		ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="Wavelength", Emode="Elastic")
	    CropWorkspace(InputWorkspace=wsname, OutputWorkspace=wsname, XMin=0.7, XMax=10.35)
		# normalise by monitor
		wsname_mon = self._proccess_monitor(wsname)
		NormaliseToMonitor(InputWorkspace=wsname, OutputWorkspace=wsname, MonitorWorkspace=monitor)
		NormaliseToMonitor(InputWorkspace=wsname, OutputWorkspace=wsname, MonitorWorkspace=monitor,
						   IntegrationRangeMin=0.7, IntegrationRangeMax=10.35)
		ReplaceSpecialValues(InputWorkspace=wsname, OutputWorkspace=wsname, NaNValue=0.0, NaNError=0.0,
							 InfinityValue=0.0, InfinityError=0.0)
		# converty to TOF
		ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target="TOF", EMode="Elastic")
		return wsname

	def focus(self, wsname):
		# lazily load grouping workspace
		if self.grp_ws is None:
			self.grp_ws = LoadDetectorsGroupingFile(InputFile=self.grp_ws_fpath, InputWorkspace=wsname,
													OutputWorkspace="wish_grp_ws")
		ConvertUnits(InputWorkspace=wsname, OutputWorkspace=wsname, Target='dSpacing')
		return DiffractionFocussing(InputWorkspace=wsname, OutputWorkspace=f'{wsname}_foc',
									GroupingWorkspace=self.grp_ws)

	# def create_empty
	#
	# def create_vanadium
	#
	# def abs_corr
	#
	# def scale_abs_units

	@staticmethod
	def _process_monitor(wsname, nbreaks=70, npts_smooth=40):
		wsname_mon = wsname + "_mon"
		ExtractSingleSpectrum(InputWorkspace=wsname,OutputWorkspace=wsname_mon, WorkspaceIndex=3)
		ConvertToDistribution(wsname_mon)
		# mask bragg edges
		for xmin, xmax in [(4.57, 4.76), (3.87,4.12), (2.75,2.91), (2.24,2.50)]:
			MaskBins(InputWorkspace=wsname_mon,OutputWorkspace=wsname_mon,XMin=xmin, XMax=xmax)
		# fit background
		SplineBackground(InputWorkspace=wsname_mon, OutputWorkspace=wsname_mon, WorkspaceIndex=0, NCoeff=nbreaks)
		ws_mon = SmoothData(InputWorkspace=wsname_mon, OutputWorkspace=wsname_mon, NPoints=npts_smooth)
		ConvertFromDistribution(wsname_mon)
		return wsname_mon

	@staticmethod
	def _crop_focussed_in_dspac(ws_foc):
		xmins = cycle([0.8, 0.5, 0.5, 0.4, 0.35])
		xmaxs = cycle([53.3, 13.1, 7.77, 5.86, 4.99])
		for ispec in range(ws_foc.getNumberHistograms()):
			ws_foc = MaskBins(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(),
							  XMin=0, XMax=next(xmins), InputWorkspaceIndexSet=ispec)
			ws_foc = MaskBins(InputWorkspace=ws_foc, OutputWorkspace=ws_foc.name(),
							  XMin=next(xmaxs), XMax=100, InputWorkspaceIndexSet=ispec)

	@staticmethod
	def _combine_focussed_banks(ws_foc):
		ws_foc_1to5 = ExtractSpectra(InputWorkspace=ws_foc, OutputWorkspace=f"{ws_foc.name()}_1to5",
									 WorkspaceIndexList=range(0, 5))
		ws_foc_6to10 = ExtractSpectra(InputWorkspace=ws_foc, OutputWorkspace=f"{ws_foc.name()}_6to10",
									  WorkspaceIndexList=range(5, 10))
		# ensure bin edges in equivalent banks are the same (could be different due to calibration)
		npanels = ws_foc_1to5.getNumberHistograms()
		for ispec in range(npanels):
			ws_foc_6to10.applyBinEdgesFromAnotherWorkspace(ws_foc_1to5, ispec, ispec)  # make sure common bin edges
		Plus(LHSWorkspace=ws_foc_1to5, RHSWorkspace=ws_foc_6to10, OutputWorkspace=ws_foc.name())
