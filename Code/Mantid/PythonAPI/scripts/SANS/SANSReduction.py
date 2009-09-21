###########################################################################
# This is the template analysis script for reducing the SANS data.        #
# It has place holders so that the appropriate data can be filled in at   #
# run time by the SANS GUI                                                #
#                                                                         #
#  Authors: Russell Taylor, Tessella plc (Original script)                #
#           Martyn Gigg, Tessella plc (Adapted to GUI use)                #
#	    with major input from Richard Heenan                              #
###########################################################################
#
#
# README: 
# This script contains a list of function definitions to perform the SANS data reduction.
# It was primarily designed as a back end to the GUI. When being used from the GUI, MantidPlot
# automatically replaces all of the information at the top of this module
#  

#
# Set up for cycle 09/02 for which the spectra are now by rows, right to left 
# from top right, and start at 9 after 8 monitors !
#
import SANSUtility
import math

# ---------------------------- CORRECTION INPUT -----------------------------------------
# The information between this line and the other '-----' delimiter needs to be provided
# for the script to function. From the GUI, the tags will be replaced by the appropriate
# information. 

# The tags get replaced by input from the GUI
# The workspaces
SCATTER_SAMPLE = '|SCATTERSAMPLE|'
SCATTER_CAN = '|SCATTERCAN|'
TRANS_SAMPLE = '|TRANSMISSIONSAMPLE|'
TRANS_CAN = '|TRANSMISSIONCAN|'
DIRECT_SAMPLE = '|DIRECTSAMPLE|'

# Now the mask string (can be empty)
MASKSTRING = '|MASKSTRING|'

# Instrument information
INSTR_DIR = '|INSTRUMENTPATH|'
INSTR_NAME = '|INSTRUMENTNAME|'
# Beam centre in mm
XBEAM_CENTRE = |XBEAM|/1000.
YBEAM_CENTRE = |YBEAM|/1000.

# Analysis tab values
RMIN = |RADIUSMIN|/1000.0
RMAX = |RADIUSMAX|/1000.0
WAV1 = |WAVMIN|
WAV2 = |WAVMAX|
DWAV = |WAVDELTA|
WAVBINNING = str(WAV1) + "," + str(DWAV) + "," + str(WAV2)
Q1 = |QMIN|
Q2 = |QMAX|
DQ = |QDELTA|
QXY2 = |QXYMAX|
DQXY = |QXYDELTA|
DIRECT_BEAM_FILE = '|DIRECTFILE|'
# This indicates whether a 1D or a 2D analysis is performed
CORRECTION_TYPE = '|ANALYSISTYPE|'
# Component positions
SAMPLE_Z_CORR = |SAMPLEZOFFSET|/1000.

# Scaling values
RESCALE = |SCALEFACTOR|*100.0
SAMPLE_GEOM = |GEOMID|
SAMPLE_WIDTH = |SAMPLEWIDTH|
SAMPLE_HEIGHT = |SAMPLEHEIGHT|
SAMPLE_THICKNESS = |SAMPLETHICK| 

# These values are used for the start and end bins for FlatBackground removal.
###############################################################################################
# RICHARD'S NOTE FOR SANS2D: these may need to vary with chopper phase and detector distance !
# !TASK! Put the values in the mask file if they need to be different ?????
##############################################################################################
# The GUI will replace these with default values of
# LOQ: 31000 -> 39000
# S2D: 85000 -> 100000
BACKMON_START = |BACKMONSTART|
BACKMON_END = |BACKMONEND|

# The detector bank to look at. The GUI has an options box to select the detector to analyse. 
# The spectrum numbers are deduced from the name within the |DETBANK| tag. Names are from the 
# instrument definition file
# LOQ: HAB or main-detector-bank
# S2D: front-detector or rear-detector 
DETBANK = '|DETBANK|'

# The monitor spectrum taken from the GUI. Is this still necessary?? or can I just deduce
# it from the instrument name 
MONITORSPECTRUM = |MONSPEC|

# Detector position information for SANS2D
FRONT_DET_RADIUS = 306.0
FRONT_DET_DEFAULT_SD_M = 4.0
FRONT_DET_DEFAULT_X_M = 1.1
REAR_DET_DEFAULT_SD_M = 4.0

# LOG files for SANS2D will have these encoder readings  
FRONT_DET_Z = |ZFRONTDET|
FRONT_DET_X = |XFRONTDET|
FRONT_DET_ROT = |ROTFRONTDET|
REAR_DET_Z = |ZREARDET|
# Rear_Det_X  Will Be Needed To Calc Relative X Translation Of Front Detector 
REAR_DET_X = |XREARDET|

# MASK file stuff ==========================================================
# correction terms to SANS2d encoders - store in MASK file ?
FRONT_DET_Z_CORR = |ZCORRFRONTDET| 
FRONT_DET_Y_CORR = |YCORRFRONTDET| 
FRONT_DET_X_CORR = |XCORRFRONTDET| 
FRONT_DET_ROT_CORR = |ROTCORRFRONTDET|
REAR_DET_Z_CORR = |ZCORREARDET| 
REAR_DET_X_CORR = |XCORREARDET| 

# Previous transmission
USE_PREV_TRANS = |USEPREVTRANS|
#------------------------------- End of input section --------------------------------------------------

# Transmission variables for SANS2D. The CalculateTransmission algorithm contains the defaults
# for LOQ so these are not used for LOQ
TRANS_WAV1 = 2.0
TRANS_WAV2 = 14.0
TRANS_UDET_MON = 2
TRANS_UDET_DET = 3

# Instrument specific information using function in utility file
DIMENSION, SPECMIN , SPECMAX, BACKMON_START, BACKMON_END = SANSUtility.GetInstrumentDetails(INSTR_NAME, DETBANK)

#################################### Correct by the transmission function ############################################
def CalculateTransmissionCorrection(trans_raw, DIRECT_SAMPLE, lambdamin, dlambda, lambdamax,outputworkspace):
	if trans_raw == '' or DIRECT_SAMPLE == '':
		return False

	if USE_PREV_TRANS == True:
		lmin = TRANS_WAV1
		lmax = TRANS_WAV2
		wavbin = str(TRANS_WAV1) + ',' + str(dlambda) + ',' + str(TRANS_WAV2)
	else:
		lmin = lambdamin
		lmax = lambdamax
		wavbin = str(lambdamin) + ',' + str(dlambda) + ',' + str(lambdamax)

	mtd.deleteWorkspace(outputworkspace)
	if INSTR_NAME == 'LOQ':
		# Change the instrument definition to the correct one in the LOQ case
		LoadInstrument(trans_raw, INSTR_DIR + "/LOQ_trans_Definition.xml")
		LoadInstrument(DIRECT_SAMPLE, INSTR_DIR + "/LOQ_trans_Definition.xml")
		trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw, '1,2', BACKMON_START, BACKMON_END, wavbin, True)
		direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(DIRECT_SAMPLE, '1,2', BACKMON_START, BACKMON_END, wavbin, True)
		CalculateTransmission(trans_tmp_out,direct_tmp_out, outputworkspace)
	else:
		trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw, '1,2', BACKMON_START, BACKMON_END, wavbin, False) 
		direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(DIRECT_SAMPLE, '1,2', BACKMON_START, BACKMON_END, wavbin, False)
		CalculateTransmission(trans_tmp_out,direct_tmp_out, outputworkspace, TRANS_UDET_MON, TRANS_UDET_DET, lmin, lmax)
	# Remove temopraries
	mantid.deleteWorkspace(trans_tmp_out)
	mantid.deleteWorkspace(direct_tmp_out)

	# If we want to use previous transmission then calculate to full range and rebin
	if USE_PREV_TRANS == True:
		wavbin = str(lambdamin) + ',' + str(dlambda) + ',' + str(lambdamax)
		Rebin(outputworkspace, outputworkspace, wavbin)
		
	return True
#######################################################################################################################

############################## Setup the component positions ##########################################################
def SetupComponentPositions(detector, dataws, xbeam, ybeam):
	### Sample correction #### 
	# Put the components in the correct place
	# The sample
	MoveInstrumentComponent(dataws, 'some-sample-holder', Z = SAMPLE_Z_CORR, RelativePosition="1")
	
	# The detector
	if INSTR_NAME == 'LOQ':
		xshift = (317.5/1000.) - xbeam
		yshift = (317.5/1000.) - ybeam
		MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, RelativePosition="1")
		# LOQ instrument description has detector at 0.0, 0.0
		return [xshift, yshift], [xshift, yshift] 
		## !TASK! HAB
	else:
  		if detector == 'front-detector':
			rotateDet = (-FRONT_DET_ROT - FRONT_DET_ROT_CORR)
			RotateInstrumentComponent(dataws, detector,X="0.",Y="1.0",Z="0.",Angle=rotateDet)
   			RotRadians = math.pi*(FRONT_DET_ROT + FRONT_DET_ROT_CORR)/180.
			xshift = (REAR_DET_X + REAR_DET_X_CORR - FRONT_DET_X - FRONT_DET_X_CORR + FRONT_DET_RADIUS*math.sin(RotRadians ) )/1000. - FRONT_DET_DEFAULT_X_M - xbeam
			yshift = (FRONT_DET_X_CORR /1000.  - ybeam)
			# default in instrument description is 23.281m - 4.000m from sample at 19,281m !
			# need to add ~58mm to det1 to get to centre of detector, before it is rotated.
			zshift = (FRONT_DET_Z + FRONT_DET_Z_CORR + FRONT_DET_RADIUS*(1 - math.cos(RotRadians)) )/1000. - FRONT_DET_DEFAULT_SD_M
			MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
			return [0.0, 0.0], [0.0, 0.0]
		else:
   			xshift = -xbeam
			yshift = -ybeam
			zshift = (REAR_DET_Z + REAR_DET_Z_CORR)/1000. - REAR_DET_DEFAULT_SD_M
   			MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
			return [0.0,0.0], [xshift, yshift]
		# Should never reach here
        return [0.0, 0.0], [0.0, 0.0]
################################################################################################

#################################### Main correction function ###################################
def Correct(sample_raw, trans_final, final_result, wav_start, wav_end, maskpt_rmin = [0.0,0.0], maskpt_rmax = [0.0,0.0], FindingCentre = False):
	'''Performs the data reduction steps'''
	global MONITORSPECTRUM, SPECMIN, SPECMAX
	if INSTR_NAME == "SANS2D":
		sample_run = SCATTER_SAMPLE.split('_')[0]
		if int(sample_run) < 568:
			MONITORSPECTRUM = 73730
			monstart = 0
			if DETBANK == 'rear-detector':
				SPECMIN = DIMENSION*DIMENSION + 1 + monstart
				SPECMAX = DIMENSION*DIMENSION*2 + monstart
			else:
				SPECMIN = 1 + monstart
				SPECMAX = DIMENSION*DIMENSION + monstart			
				
	############################# Setup workspaces ######################################
	monitorWS = "Monitor"
	# Get the monitor ( StartWorkspaceIndex is off by one with cropworkspace)
	CropWorkspace(sample_raw, monitorWS, StartWorkspaceIndex = str(MONITORSPECTRUM - 1), EndWorkspaceIndex = str(MONITORSPECTRUM - 1))
	if INSTR_NAME == 'LOQ':
		RemoveBins(monitorWS, monitorWS, '19900', '20500', Interpolation="Linear")
		# Remove flat background
	FlatBackground(monitorWS, monitorWS, '0', BACKMON_START, BACKMON_END)
	# Get the bank we are looking at
	CropWorkspace(sample_raw, final_result, StartWorkspaceIndex = (SPECMIN - 1), EndWorkspaceIndex = str(SPECMAX - 1))
	#####################################################################################
	
	########################## Masking  ################################################
	# Mask the corners and beam stop if radius parameters are given and we are not performing
	# the centre finding algorithm
	if FindingCentre == False:
		if RMIN > 0.0: 
			SANSUtility.MaskInsideCylinder(final_result, RMIN, maskpt_rmin[0], maskpt_rmin[1])
		if RMAX > 0.0:
			SANSUtility.MaskOutsideCylinder(final_result, RMAX, maskpt_rmax[0], maskpt_rmax[1])
		# Mask other requested spectra that are given in the GUI
		speclist = SANSUtility.ConvertToSpecList(MASKSTRING, SPECMIN, DIMENSION)
		SANSUtility.MaskBySpecNumber(final_result, speclist)
	####################################################################################
	
	######################## Unit change and rebin #####################################
	# Convert all of the files to wavelength and rebin
	# ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
	ConvertUnits(monitorWS, monitorWS, "Wavelength")
	wavbin =  str(wav_start) + "," + str(DWAV) + "," + str(wav_end)
	Rebin(monitorWS, monitorWS,wavbin)
	ConvertUnits(final_result,final_result,"Wavelength")
	Rebin(final_result,final_result,wavbin)
	####################################################################################

	####################### Correct by incident beam monitor ###########################
	# At this point need to fork off workspace name to keep a workspace containing raw counts
	tmpWS = "temporary_workspace"
	Divide(final_result, monitorWS, tmpWS)
	mantid.deleteWorkspace(monitorWS)
	###################################################################################

	############################ Transmission correction ##############################
	if trans_final != '':
		Divide(tmpWS, trans_final, tmpWS)
	##################################################################################   
	
	############################ Efficiency correction ################################
	CorrectToFile(tmpWS, DIRECT_BEAM_FILE, tmpWS, "Wavelength", "Divide")
	###################################################################################
	
	# Scale by the sample volume
	SANSUtility.ScaleByVolume(tmpWS, RESCALE, SAMPLE_GEOM, SAMPLE_WIDTH, SAMPLE_HEIGHT, SAMPLE_THICKNESS)
	
	################################ Correction in Q space ############################
	# 1D
	if CORRECTION_TYPE == '1D':
		# Convert to Q
		ConvertUnits(tmpWS,tmpWS,"MomentumTransfer")
		ConvertUnits(final_result,final_result,"MomentumTransfer")
		
		# Need to mark the workspace as a distribution at this point to get next rebin right
		ws = mantid.getMatrixWorkspace(tmpWS)
		ws.isDistribution(True)
		
		# Rebin to desired Q bins
		q_bins = str(Q1) + "," + str(DQ) + "," + str(Q2)
		Rebin(final_result, final_result, q_bins)
		# Calculate the solid angle corrections
		SolidAngle(tmpWS,"solidangle")
		RebinPreserveValue("solidangle", "solidangle", q_bins)
		Rebin(tmpWS, tmpWS, q_bins)

		if FindingCentre == True:
			Divide(tmpWS,"solidangle",tmpWS)
			mantid.deleteWorkspace("solidangle")
			PoissonErrors(tmpWS, final_result, final_result)
			mantid.deleteWorkspace(tmpWS)
			return
		
		# Sum all spectra
		SumSpectra(final_result,final_result)
		SumSpectra(tmpWS,tmpWS)
		SumSpectra("solidangle","solidangle")
	
		# Correct for solidangle
		Divide(tmpWS,"solidangle",tmpWS)
		mantid.deleteWorkspace("solidangle")

		# Now put back the fractional error from the raw count workspace into the result
		PoissonErrors(tmpWS, final_result, final_result)
	# 2D	
	else:
		# Run 2D algorithm
		Qxy(tmpWS, final_result, QXY2, DQXY)

	mantid.deleteWorkspace(tmpWS)
	return
############################# End of Correct function ####################################################

########################################## Reduction #####################################################
def InitReduction(run_ws, beamcoords, EmptyCell):
	if run_ws == '':
		return '', '', [0.0,0.0], [0.0, 0.0]
	
	# Put the components in the correct positions
	maskpt_rmin, maskpt_rmax = SetupComponentPositions(DETBANK, run_ws, beamcoords[0], beamcoords[1])

	# Calculate transmission first if a run is available 
	# It is only calculated once across the whole wavelength range and reused later
	if EmptyCell == True:
		trans_input = TRANS_CAN
		trans_suffix = 'can'
		final_workspace = "temp_can_holder"
	else:
		trans_input = TRANS_SAMPLE
		trans_suffix = 'sample'
        # Final workspace is named run+{bank}+_correctiontype for the sample run
		final_workspace = run_ws.split('_')[0]
		if DETBANK == 'front-detector':
			final_workspace += 'front'
		elif DETBANK == 'rear-detector':
			final_workspace += 'rear'
		elif DETBANK == 'main-detector-bank':
			final_workspace += 'main'
		else:
			final_workspace += 'HAB'
		final_workspace += '_' + CORRECTION_TYPE

	# Centre reduction works a lot better when pressing the button again with this here
	mtd.deleteWorkspace(run_ws.split('_')[0] + 'rear_centre-reduced')

	trans_final = trans_input.split('_')[0] + "_trans_" + trans_suffix
	have_sample_trans = CalculateTransmissionCorrection(trans_input, DIRECT_SAMPLE, WAV1, DWAV, WAV2, trans_final)
	if have_sample_trans == False:
		trans_final = ''
	
	return trans_final, final_workspace, maskpt_rmin, maskpt_rmax


# This runs a specified range
def WavRangeReduction(sample_setup, can_setup, wav_start, wav_end, FindingCentre = False):
	# Run correction function
	if FindingCentre == True:
		final_workspace = sample_setup[1].split('_')[0] + '_centre-reduced'
	else:
		final_workspace = sample_setup[1] + '_' + str(wav_start) + '_' + str(wav_end)
		
	Correct(SCATTER_SAMPLE, sample_setup[0], final_workspace, wav_start, wav_end, sample_setup[2], sample_setup[3], FindingCentre)
	if can_setup[1] != '':
		# Run correction function
		Correct(SCATTER_CAN, can_setup[0], can_setup[1], wav_start, wav_end, can_setup[2], can_setup[3], FindingCentre)
		Minus(final_workspace, can_setup[1], final_workspace)
		mantid.deleteWorkspace(can_setup[1])
	
	# Replaces NANs with zeroes
	ReplaceSpecialValues(InputWorkspace=final_workspace,OutputWorkspace=final_workspace,NaNValue="0",InfinityValue="0")
	# Crop Workspace to remove leading and trailing zeroes
	if CORRECTION_TYPE == '1D':
                SANSUtility.StripEndZeroes(final_workspace)
	return final_workspace
############################################################################################################################


####################### A function to calculate residual for centre finding ################################################

try:
	import scipy.optimize
	# These variables keep track of the centre coordinates that have been used so that we can calculate a relative shift of the
	# detector
	XVAR_PREV = 0.0
	YVAR_PREV = 0.0
	ITER_NUM = 0

	def reportProgress(xk):
		global ITER_NUM
		ITER_NUM += 1
		mantid.sendLogMessage("::SANS::Iteration: " + str(ITER_NUM))


	def Residuals(vars, *args):
		'''Compute the value of (L-R)^2+(U-D)^2 a circle split into four quadrants (cones really)'''
		# *args indicates a list of arguments.
		global XVAR_PREV, YVAR_PREV
		xcentre = vars[0]
		ycentre= vars[1]
		
		xshift = xcentre - XVAR_PREV
		yshift = ycentre - YVAR_PREV
		XVAR_PREV = xcentre
		YVAR_PREV = ycentre

		# Do the correction
		if xshift <> 0.0 or yshift <> 0.0:
			MoveInstrumentComponent(SCATTER_SAMPLE, ComponentName = DETBANK, X = str(xshift), Y = str(yshift), RelativePosition="1")
			if SCATTER_CAN != '':
				MoveInstrumentComponent(SCATTER_CAN, ComponentName = DETBANK, X = str(xshift), Y = str(yshift), RelativePosition="1")
	
		# Arguments 0 and 1 are the sample and can setup details
		ws_togroup = WavRangeReduction(args[0], args[1], WAV1, WAV2, FindingCentre=True)
		rlow = args[2]
		rupp = args[3]
		# The workspace that we want to group is the output of the sample reduction
		quad_ws = SANSUtility.GroupIntoQuadrants(ws_togroup, 0.0, 0.0, rlow, rupp)
		return SANSUtility.CalculateResidue(quad_ws)

	def FindBeamCentre(rlow, rupp, MaxIter = 10, xstart = None, ystart = None):
		global XVAR_PREV, YVAR_PREV, ITER_NUM
		
		# Clear any previous workspaces (Note that a log message will appear saying they don't exists
		# if there is not but execution will continue)
		mtd.deleteWorkspace('Left')
		mtd.deleteWorkspace('Right')
		mtd.deleteWorkspace('Up')
		mtd.deleteWorkspace('Down')

		if xstart == None or ystart == None:
			# If a starting point is not provided, do a quick sweep to find the maximum count as an 
			# approximate place to start the search
			Integration(SCATTER_SAMPLE, 'tmp')
			t = mtd.getMatrixWorkspace("tmp")
			nhist = t.getNumberHistograms()
			maxcount = -1.0
			idet = -1
			for idx in range(0, nhist):
				try:
					d = t.getDetector(idx)
					if d.isMonitor() == True:
						continue
				except:
					continue
				ycount = t.readY(idx)[0]
				if ycount > maxcount:
					maxcount = ycount
					idet = idx
				if idx > SPECMAX:
					break
			mtd.deleteWorkspace('tmp')
			detpos = mtd.getMatrixWorkspace(SCATTER_SAMPLE).getDetector(idet).getPos()
			XVAR_PREV = detpos.getX()
			YVAR_PREV = detpos.getY()
		else:
			XVAR_PREV = xstart
			YVAR_PREV = ystart

		# Initialize the workspace with the starting coordinates. (Note that this moves the detector to -x,-y)
		scatter_setup = InitReduction(SCATTER_SAMPLE, [XVAR_PREV, YVAR_PREV], EmptyCell = False)
		can_setup = InitReduction(SCATTER_CAN, [XVAR_PREV, YVAR_PREV], EmptyCell = True)

		XVAR_PREV = -XVAR_PREV
		YVAR_PREV = -YVAR_PREV
		# There's a bug in the scipy.optimize.fmin function that skips the callback function for the first iteration
		ITER_NUM = 0
		reportProgress([0,0])
		coords = scipy.optimize.fmin(Residuals, [XVAR_PREV, YVAR_PREV], (scatter_setup, can_setup, rlow, rupp),xtol=1e-2,maxiter=MaxIter, callback = reportProgress)
		
		# Tidy up
#		mtd.deleteWorkspace('Left')
#		mtd.deleteWorkspace('Right')
#		mtd.deleteWorkspace('Up')
#		mtd.deleteWorkspace('Down')
#		mtd.deleteWorkspace(scatter_setup[1] + '_' + str(WAV1) + '_' + str(WAV2))
#		mtd.deleteWorkspace(SCATTER_SAMPLE)
#		if SCATTER_CAN != '':
#			mtd.deleteWorkspace(SCATTER_CAN)
		
		# The coordinates returned are the position of the detector so the beam centre is -coords
		return -coords
except:
	def FindBeamCentre(rlow, rupp, MaxIter = 15, xstart = None, ystart = None):
		print 'This function requires the scipy package to be installed.'
		return [0,0]
############################################################################################################################

################################### Work begins here #####################################################################
# The GUI appends it's own commands here so if you are using this as a script then the best thing is to use this as a template
# and copy it to another file and execute that, making sure that the details at the top are appropriately filled in.

### Example code:
# The beam finding alters the raw data workspace so be careful before performing other analyses straight after
# using this
#beamcoords = FindBeamCentre(rlow = 0.06, rupp= 0.28, MaxIter = 10)
#print beamcoords

# Remove and reload all workspaces before doing corrections
#mtd.clear();

# Do the initialization
#sample_setup = InitReduction(SCATTER_SAMPLE, [0.082, -0.193], False)
#can_setup = InitReduction(SCATTER_CAN, [0.082, -0.193], True)

### Corrections
# FullWavRangeReduction(sample_setup, can_setup)
### Or for a smaller lambda range 
# WavRangeReduction(sample_setup, can_setup, 2, 4)

