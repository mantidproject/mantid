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

################################# Input for SANS corrections ###########################################

# The tags get replaced by input from the GUI
# The workspaces
scatter_sample = '|SCATTERSAMPLE|'
scatter_can = '|SCATTERCAN|'
trans_sample = '|TRANSMISSIONSAMPLE|'
trans_can = '|TRANSMISSIONCAN|'
direct_sample = '|DIRECTSAMPLE|'

# Now the mask string (can be empty)
maskstring = '|MASKSTRING|'

# Instrument information
instr_dir = '|INSTRUMENTPATH|'
instr_name = '|INSTRUMENTNAME|'
# Beam centre in mm
xbeam_maskfile = |XBEAM|
ybeam_maskfile = |YBEAM|

# Analysis tab values
rmin = |RADIUSMIN|/1000.0
rmax = |RADIUSMAX|/1000.0
wav1 = |WAVMIN|
wav2 = |WAVMAX|
dwav = |WAVDELTA|
fullwavbin = str(wav1) + "," + str(dwav) + "," + str(wav2)
q1 = |QMIN|
q2 = |QMAX|
dq = |QDELTA|
qxy2 = |QXYMAX|
dqxy = |QXYDELTA|
direct_beam_file = '|DIRECTFILE|'
# This indicates whether a 1D or a 2D analysis is performed
correction_type = '|ANALYSISTYPE|'
# Component positions
sample_z_corr = |SAMPLEZOFFSET|/1000.

# Scaling values
rescale = |SCALEFACTOR|*100.0
sample_geom = |GEOMID|
sample_width = |SAMPLEWIDTH|
sample_height = |SAMPLEHEIGHT|
sample_thickness = |SAMPLETHICK| 

# These values are used for the start and end bins for FlatBackground removal.
###############################################################################################
# RICHARD'S NOTE FOR SANS2D: these may need to vary with chopper phase and detector distance !
# !TASK! Put the values in the mask file if they need to be different ?????
##############################################################################################
# The GUI will replace these with default values of
# LOQ: 31000 -> 39000
# S2D: 85000 -> 100000
backmon_start = |BACKMONSTART|
backmon_end = |BACKMONEND|

# The detector bank to look at. The GUI has an options box to select the detector to analyse. 
# The spectrum numbers are deduced from the name within the |DETBANK| tag. Names are from the 
# instrument definition file
# LOQ: HAB or main-detector-bank
# S2D: front-detector or rear-detector 
detbank = '|DETBANK|'

# The monitor spectrum taken from the GUI. Is this still necessary?? or can I just deduce
# it from the instrument name 
monitorspectrum = |MONSPEC|
##############################################################################################

# Transmission variables for SANS2D. The CalculateTransmission algorithm contains the defaults
# for LOQ so these are not used for LOQ
trans_wav1 = 2.0
trans_wav2 = 14.0
trans_udet_mon = 2
trans_udet_det = 3

# Instrument specific information
dimension = ''
if instr_name == 'LOQ':
	dimension = 128
	if detbank == 'main-detector-bank':
		specmin = 3
		specmax = 16386
	else:
		specmin = 16387
		specmax = 17792
	# Flat background values
	backmon_start = 31000
	backmon_end = 39000
else:
	dimension = 192
	# This is the number of monitors before the first set of detectors 
	monstart = 8
	if detbank == 'front-detector':
		specmin = dimension*dimension + 1 + monstart
		specmax = dimension*dimension*2 + monstart
	else:
		specmin = 1 + monstart
		specmax = dimension*dimension + monstart
	# Flat background values
	backmon_start = 85000
	backmon_end = 100000

#################################### Correct by the transmission ###############################
def CalculateTransmissionCorrection(trans_raw, direct_sample, wavbin, outputworkspace):
	if trans_raw == '' or direct_sample == '':
		return False
	
	if instr_name == 'LOQ':
		# Change the instrument definition to the correct one in the LOQ case
		LoadInstrument(trans_raw, instr_dir + "/LOQ_trans_Definition.xml")
		LoadInstrument(direct_sample, instr_dir + "/LOQ_trans_Definition.xml")
		trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw, '1,2', backmon_start, backmon_end, wavbin, True)
		direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_sample, '1,2', backmon_start, backmon_end, wavbin, True)
		CalculateTransmission(trans_tmp_out,direct_tmp_out, outputworkspace)
	else:
		trans_tmp_out = SANSUtility.SetupTransmissionWorkspace(trans_raw, '1,2', backmon_start, backmon_end, wavbin, False) 
		direct_tmp_out = SANSUtility.SetupTransmissionWorkspace(direct_sample, '1,2', backmon_start, backmon_end, wavbin, False)
		CalculateTransmission(trans_tmp_out,direct_tmp_out, outputworkspace, trans_udet_mon, trans_udet_det, trans_wav1, trans_wav2)
	
	mantid.deleteWorkspace(trans_tmp_out)
	mantid.deleteWorkspace(direct_tmp_out)
	return True
################################################################################################

############################## Setup the component positions ###################################
def SetupComponentPositions(detector, dataws, xbeam, ybeam):
	### Sample correction #### 
	# Put the components in the correct place
	# 1. The sample
	MoveInstrumentComponent(dataws, 'some-sample-holder', Z = sample_z_corr, RelativePosition="1")

	if instr_name == 'LOQ':
		xshift = (317.5 - xbeam)/1000.
		yshift = (317.5 - ybeam)/1000.
		MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, RelativePosition="1")
		# LOQ instrument description has detector at 0.0, 0.0
		return [xshift, yshift]
		## !TASK! HAB
	else:
		# Move the detector to the correct place
		# hardware, rotation radius of front det
		Front_Det_Radius = 306.
		front_det_default_sd_m = 4.0
		front_det_default_x_m = 1.1
		rear_det_default_sd_m = 4.0
		# LOG files for SANS2d will have these encoder readings  ============================================
		# TASK Get the current values on the workspace
		Front_Det_z = 4500.
		Front_Det_x = -1000.
		Front_Det_Rot = -12.
		Rear_Det_z = 6000.
		# Rear_Det_x  will be needed to calc relative X translation of front detector 
		Rear_Det_x = 100.
        
		# MASK file stuff ==========================================================
		# correction terms to SANS2d encoders - store in MASK file ?
		Rear_Det_z_corr = 58.
		Rear_Det_x_corr = 0.0
		Front_Det_z_corr = 47.
		Front_Det_x_corr = -33.
		Front_Det_y_corr = -20.
		Front_Det_Rot_corr = 0.0
  
  		if detector == 'front-detector':
			rotateDet = (-Front_Det_Rot -  Front_Det_Rot_corr)
			RotateInstrumentComponent(dataws, detector,X="0.",Y="1.0",Z="0.",Angle=rotateDet)
   			RotRadians = math.pi*(Front_Det_Rot + Front_Det_Rot_corr)/180.
			xshift = (Rear_Det_x + Rear_Det_x_corr - Front_Det_x - Front_Det_x_corr + Front_Det_Radius*math.sin(RotRadians ) )/1000. - front_det_default_x_m - xbeam/1000.
			yshift = (Front_Det_y_corr /1000.  - ybeam/1000.)
			# default in instrument description is 23.281m - 4.000m from sample at 19,281m !
			# need to add ~58mm to det1 to get to centre of detector, before it is rotated.
			zshift = (Front_Det_z + Front_Det_z_corr + Front_Det_Radius*(1 - math.cos(RotRadians)) )/1000.  -front_det_default_sd_m
			MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
			return [-xshift, -yshift]
		else:
   			xshift = -xbeam/1000.
			yshift = -ybeam/1000.
			zshift = (Rear_Det_z + Rear_Det_z_corr)/1000. - 4.000
   			MoveInstrumentComponent(dataws, detector, X = xshift, Y = yshift, Z = zshift, RelativePosition="1")
			return [-xshift, -yshift]
		# Should never reach here
        return [0.0, 0.0]
################################################################################################

#################################### Main correction function ##################################
def Correct(sample_raw, trans_final, final_result, maskcentre = [0.0,0.0], FindingCentre = False):
	'''Performs the data reduction steps'''
	global monitorspectrum, specmin, specmax
	if instr_name == "SANS2D":
		sample_run = scatter_sample.split('_')[0]
		if int(sample_run) < 568:
			monitorspectrum = 73730
			monstart = 0
			if detbank == 'rear-detector':
				specmin = dimension*dimension + 1 + monstart
				specmax = dimension*dimension*2 + monstart
			else:
				specmin = 1 + monstart
				specmax = dimension*dimension + monstart			
				
	############################# Setup workspaces ############################
	monitorWS = "Monitor"
	# Get the monitor ( StartSpectrum is off by one with cropworkspace)
	CropWorkspace(sample_raw, monitorWS, StartWorkspaceIndex = str(monitorspectrum - 1), EndWorkspaceIndex = str(monitorspectrum - 1))
	if instr_name == 'LOQ':
		RemoveBins(monitorWS, monitorWS, '19900', '20500', Interpolation="Linear")
	# Remove flat background
	FlatBackground(monitorWS, monitorWS, '0', backmon_start, backmon_end)
	# Get the bank we are looking at. Crop uses workspace indices so StartSpectrum is a bit
	# confusing here but it's off by one
	CropWorkspace(sample_raw, final_result, StartWorkspaceIndex = (specmin - 1), EndWorkspaceIndex = str(specmax - 1))
	###########################################################################
	
	########################## Masking  #######################################
	# Mask the corners and beam stop if radius parameters are given:
	if FindingCentre == False:
		if rmin > 0.0: 
			SANSUtility.MaskInsideCylinder(final_result, rmin, maskcentre[0], maskcentre[1])
		if rmax > 0.0:
			SANSUtility.MaskOutsideCylinder(final_result, rmax, maskcentre[0], maskcentre[1])
		# Mask other requested spectra that are given in the GUI
		speclist = SANSUtility.ConvertToSpecList(maskstring, specmin, dimension)
		SANSUtility.MaskBySpecNumber(final_result, speclist)
	############################################################################
	
	######################## Unit change and rebin #####################################
	# Convert all of the files to wavelength and rebin
	# ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
	ConvertUnits(monitorWS, monitorWS, "Wavelength")
	Rebin(monitorWS, monitorWS,fullwavbin)
	ConvertUnits(final_result,final_result,"Wavelength")
	Rebin(final_result,final_result,fullwavbin)
	####################################################################################

	####################### Correct by incident beam monitor ###########################
	# At this point need to fork off workspace name to keep a workspace containing raw counts
	tmpWS = "temporary_workspace"
	Divide(final_result, monitorWS, tmpWS)
	mantid.deleteWorkspace(monitorWS)
	####################################################################################

	############################ Transmission correction ###########################
	if trans_final != '':
		Divide(tmpWS, trans_final, tmpWS)
	################################################################################     
	
	############################ Efficiency correction ##############################
	CorrectToFile(tmpWS, direct_beam_file, tmpWS, "Wavelength", "Divide")
	#################################################################################
	
	################################ Correction in Q space ##########################
	# 1D
	if correction_type == '1D':
		# Convert to Q
		ConvertUnits(tmpWS,tmpWS,"MomentumTransfer")
		ConvertUnits(final_result,final_result,"MomentumTransfer")
		
		# Need to mark the workspace as a distribution at this point to get next rebin right
		ws = mantid.getMatrixWorkspace(tmpWS)
		ws.isDistribution(True)
		
		# Rebin to desired Q bins
		q_bins = str(q1) + "," + str(dq) + "," + str(q2)
		Rebin(final_result, final_result, q_bins)
		# Calculate the solid angle corrections
		if FindingCentre == False:
			SolidAngle(tmpWS,"solidangle")
			RebinPreserveValue("solidangle", "solidangle", q_bins)
		Rebin(tmpWS, tmpWS, q_bins)

		if FindingCentre == True:
			PoissonErrors(tmpWS, final_result, final_result)
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
		# Run 2D algorithm !TASK! - saving stuff
		Qxy(tmpWS, final_result, qxy2, dqxy)

	# Replaces NANs with zeroes but would be nice to have StripLRSpecialValues type routine	
	ReplaceSpecialValues(InputWorkspace=final_result,OutputWorkspace=final_result,NaNValue="0",InfinityValue="0")
	mantid.deleteWorkspace(tmpWS)
	
	return
############################# End of Correct function ####################################################

########################################## Reduction #####################################################
def InitReduction(run_ws, emptycell):
	if run_ws == '':
		return '', '', [0,0,0.0]
	
	# Put the components in the correct positions
	beamcentre = SetupComponentPositions(detbank, run_ws, xbeam_maskfile, ybeam_maskfile)

	# Calculate transmission first if a run is available 
	# It is only calculated once across the whole wavelength range and reused later
	trans_final = trans_sample.split('_')[0] + "_trans_"
	transinput = ''
	if emptycell == True:
		trans_final += 'can'
		trans_input = trans_can
	else:
		trans_final += 'sample'
		trans_input = trans_sample
	
	have_sample_trans = CalculateTransmissionCorrection(trans_input, direct_sample, fullwavbin, trans_final)
	if have_sample_trans == False:
		trans_final = ''

	# Final workspace is named run+{bank}+_correctiontype for the sample run
	final_workspace = ''
	if emptycell == True:
		final_workspace = "temp_can_holder"
	else:
		final_workspace = run_ws.split('_')[0]
		if detbank == 'front-detector':
			final_workspace += 'front'
		elif detbank == 'rear-detector':
			final_workspace += 'rear'
		elif detbank == 'main-detector-bank':
			final_workspace += 'main'
		else:
			final_workspace += 'HAB'
		final_workspace += '_' + correction_type
	
	return trans_final, final_workspace, beamcentre

def PerformFullReduction(sample_setup, can_setup, FindingCentre = False):
	# Run correction function
	Correct(scatter_sample, sample_setup[0], sample_setup[1], sample_setup[2], FindingCentre)
	if can_setup[1] != '':
		# Run correction function
		Correct(scatter_can, can_setup[0], can_setup[1], can_setup[2], FindingCentre)
		Minus(sample_setup[1], can_setup[1], sample_setup[1])
		mantid.deleteWorkspace(can_setup[1])
		## Correct for sample/can volume ############
		SANSUtility.ScaleByVolume(sample_setup[1], rescale, sample_geom, sample_width, sample_height, sample_thickness)

############################################################################################################################


####################### A function to calculate residual for centre finding ################################################
# import scipy.optimize
# XCENTRE_PREV = 0.0
# YCENTRE_PREV = 0.0
# def Residuals(vars, *args):
# 	'''Compute the value of (L-R)^2+(U-D)^2 a circle split into four quadrants (cones really)'''
# 	global XCENTRE_PREV, YCENTRE_PREV
# 	xcentre = vars[0]
# 	ycentre= vars[1]

# 	xbeam_shift = xcentre - XCENTRE_PREV
# 	ybeam_shift = ycentre - YCENTRE_PREV
# 	XCENTRE_PREV = xcentre
# 	YCENTRE_PREV = ycentre

# 	# Do the correction
# 	MoveInstrumentComponent(scatter_sample, ComponentName = detbank, X = str(-xbeam_shift), Y = str(-ybeam_shift), RelativePosition="1")

# 	# We're not masking so centre XY argument is irrelavent
# 	PerformFullReduction(args[0], args[1], True)
	
# 	rlimit = args[2]
# 	zpos = args[3]
# 	quad_ws = SANSUtility.GroupIntoQuadrants(scatter_sample, xcentre, ycentre, zpos, rlimit)
# 	left = mtd.getMatrixWorkspace(quad_ws[0])
# 	right = mtd.getMatrixWorkspace(quad_ws[1])
# 	up = mtd.getMatrixWorkspace(quad_ws[2])
# 	dn = mtd.getMatrixWorkspace(quad_ws[3])

# 	residue = math.pow(left.readY(0)[0] - right.readY(0)[0], 2) + math.pow(up.readY(0)[0] - dn.readY(0)[0], 2)
# 	return residue

# def FindBeamCentre():
# 	global xbeam_maskfile, ybeam_maskfile
# 	# Do a quick sweep to find the maximum count as an approximate place to
# 	# start the search
# 	Integration(scatter_sample, 'tmp')
# 	t = mtd.getMatrixWorkspace("tmp")
# 	nhist = t.getNumberHistograms()
# 	maxcount = -1.0
# 	idet = -1
# 	for idx in range(0, nhist):
# 		try:
# 			d = t.getDetector(idx)
# 			if d.isMonitor() == True:
# 				continue
# 		except:
# 			continue
# 		ycount = t.readY(idx)[0]
# 		if ycount > maxcount:
# 			maxcount = ycount
# 			idet = idx
# 		if idx > specmax:
# 			break

# 	mtd.deleteWorkspace('tmp')
	
# 	print idet
# 	xmax = mtd.getMatrixWorkspace(scatter_sample).getDetector(idet).getPos().getX()
# 	ymax =  mtd.getMatrixWorkspace(scatter_sample).getDetector(idet).getPos().getY()

# 	# Store maskfile centre coords
# 	MASKFILE_X = xbeam_maskfile 
# 	MASKFILE_Y = ybeam_maskfile 
	
# 	# Use them to init the components with different 
# 	xbeam_maskfile = xmax
# 	ybeam_maskfile = ymax 
	
# 	scatter_setup = InitReduction(scatter_sample, False)
# 	can_setup = InitReduction(scatter_can, True)
# 	PerformFullReduction(scatter_setup, can_setup, True)

# 	xmax = mtd.getMatrixWorkspace(scatter_sample).getDetector(idet).getPos().getX()
# 	ymax =  mtd.getMatrixWorkspace(scatter_sample).getDetector(idet).getPos().getY()

# 	XCENTRE_PREV = xmax
# 	YCENTRE_PREV = ymax
# 	rdet_zpos = mtd.getMatrixWorkspace(scatter_sample).getDetector(idet).getPos().getZ()
# 	coords = scipy.optimize.fmin(Residuals, [xmax, ymax], (scatter_setup, can_setup, rmin, rdet_zpos))
	
# 	# Restore mask file coords
# 	xbeam_maskfile = MASKFILE_X
# 	ybeam_maskfile = MASKFILE_Y
# 	return coords

############################################################################################################################

################################### Script execution begins here ###########################################################
		
# If the script is directly executed, then jump to performing the full correction
if __name__ == '__main__':
	# Sample workspace first
	sample_setup = InitReduction(scatter_sample, False)
	can_setup = InitReduction(scatter_can, True)
	PerformFullReduction()
