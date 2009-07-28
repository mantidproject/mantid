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
# Set up for cycle 09/02 for which the spectra are now by rows, right to left 
# from top right, and start at 9 after 8 monitors !
#
import SANSUtility

################################# Input from GUI ##########################
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
xbeam_maskfile = |XBEAM|/1000.
ybeam_maskfile = |YBEAM|/1000.

# Analysis tab values
rmin = |RADIUSMIN|/1000.0
rmax = |RADIUSMAX|/1000.0
wav1 = |WAVMIN|
wav2 = |WAVMAX|
dwav = |WAVDELTA|
q1 = |QMIN|
q2 = |QMAX|
dq = |QDELTA|
qxy2 = |QXYMAX|
dqxy = |QXYDELTA|
rescale = |SCALEFACTOR|*100.0
direct_beam_file = '|DIRECTFILE|'
# This indicates whether a 1D or a 2D analysis is performed
correction_type = '|ANALYSISTYPE|'
# Component positions
sample_z_corr = |SAMPLEZOFFSET|/1000.

# hardware, rotation radius of front det
Front_Det_Radius = 306.
# LOG files for SANS2d will have these encoder readings  ============================================
# When the GUI loads the data it inspects the log files and moves the components accordingly so the
# values here get set by the GUI with the correct ones
#Front_Det_z = |FRONTDETZ|
#Front_Det_x = |FRONTDETX|
#Front_Det_Rot = |FRONTDETROT|
#Rear_Det_z = |REARDETZ|
# Rear_Det_x  will be needed to calc relative X translation of front detector 
#Rear_Det_x = |REARDETX|

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

# Instrument specfic details garnered from instrument name box on GUI and
# detector selection box
dimension = ''
if instr_name == 'LOQ':
	dimension = 128
#    monitorspectrum = 2
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
#   monitorspectrum = 2
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

##################################################################################
# Transmission variables for SANS2D. The CalculateTransmission algorithm contains the defaults
# for LOQ so these are not used for LOQ
trans_wav1 = 2.0
trans_wav2 = 14.0
trans_udet_mon = 2
trans_udet_det = 3

# !TASK!
# LOG files for SANS2D will have these encoder readings  ============================================
Front_Det_z = 4500.
Front_Det_x = -1000.
Front_Det_Rot = -12.
Rear_Det_z = 6000.
# Rear_Det_x  will be needed to calc relative X translation of front detector 
Rear_Det_x = 100.

# !TASK!
# MASK file stuff ==========================================================
#  correction terms to SANS2d encoders - store in MASK file or get from values input into GUI
Rear_Det_z_corr = 58.
Rear_Det_x_corr = 0.0
Front_Det_z_corr = 47.
Front_Det_x_corr = -33.
Front_Det_y_corr = -20.
Front_Det_Rot_corr = 0.0

#################################### Setup transmission workspace ###############################
def SetupTransmissionData(inputWS, spec_list, backmon_start, backmon_end, wavbining, loqremovebins):
	tmpWS = inputWS + '_tmp'
	if loqremovebins == True:
		RemoveBins(inputWS,tmpWS, 19900, 20500, Interpolation='Linear')
		FlatBackground(tmpWS, tmpWS, spec_list, backmon_start, backmon_end)
	else:
		FlatBackground(inputWS, tmpWS, spec_list, backmon_start, backmon_end)
	# Convert and rebin
	ConvertUnits(tmpWS,tmpWS,"Wavelength")
	Rebin(tmpWS, tmpWS, wavbining)
	return tmpWS
################################################################################################

#################################### Main correction function ##################################
def Correct(sampleWS, transWS, resultWS, suffix):
	'''Performs the data reduction steps'''
	############################# Setup workspaces ############################
	monitorWS = "Monitor"
	# Get the monitor ( StartSpectrum is off by one with cropworkspace)
	CropWorkspace(sampleWS, monitorWS, StartWorkspaceIndex = str(monitorspectrum - 1), EndWorkspaceIndex = str(monitorspectrum - 1))
	if instr_name == 'LOQ':
		RemoveBins(monitorWS, monitorWS, '19900', '20500', Interpolation="Linear")
	# Remove flat background
	FlatBackground(monitorWS, monitorWS, '0', backmon_start, backmon_end)
	# Get the bank we are looking at. Crop uses workspace indices so StartSpectrum is a bit
	# confusing here but it's off by one
	CropWorkspace(sampleWS, resultWS, StartWorkspaceIndex = (specmin - 1), EndWorkspaceIndex = str(specmax - 1))
	###########################################################################
	
	########################## Masking  #######################################
	# Mask the corners and beam stop if radius parameters are given:
	if rmin > 0.0: 
		SANSUtility.MaskInsideCylinder(resultWS, rmin)
	if rmin > 0.0 and rmax < 10.0:
		SANSUtility.MaskOutsideCylinder(resultWS, rmax)
		
	# Mask other requested spectra that are given in the GUI
	speclist = SANSUtility.ConvertToSpecList(maskstring, specmin, dimension)
	SANSUtility.MaskBySpecNumber(resultWS, speclist)
	############################################################################
	
	######################## Unit change and rebin #####################################
	# Convert all of the files to wavelength and rebin
	# ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
	wavbin = str(wav1) + "," + str(dwav) + "," + str(wav2)

	ConvertUnits(monitorWS, monitorWS, "Wavelength")
	Rebin(monitorWS, monitorWS,wavbin)
	ConvertUnits(resultWS,resultWS,"Wavelength")
	Rebin(resultWS,resultWS,wavbin)
	####################################################################################

	####################### Correct by incident beam monitor ###########################
	# At this point need to fork off workspace name to keep a workspace containing raw counts
	tmpWS = "temporary_workspace"
	Divide(resultWS, monitorWS, tmpWS)
	mantid.deleteWorkspace(monitorWS)
	####################################################################################

	########## Correct by transmission if there is one available #####################
	if transWS and direct_sample:
		trans_ws = transWS.split('_')[0] + "_trans_" + suffix
		if instr_name == 'LOQ':
			# Change the instrument definition to the correct one in the LOQ case 
			LoadInstrument(transWS, instr_dir + "/LOQ_trans_Definition.xml")
			LoadInstrument(direct_sample, instr_dir + "/LOQ_trans_Definition.xml")
			trans_tmp_out = SetupTransmissionData(transWS, '1,2', backmon_start, backmon_end, wavbin, True)
			direct_tmp_out = SetupTransmissionData(direct_sample, '1,2', backmon_start, backmon_end, wavbin, True)
			CalculateTransmission(trans_tmp_out,direct_tmp_out,trans_ws)
		else:
			trans_tmp_out = SetupTransmissionData(transWS, '1,2', backmon_start, backmon_end, wavbin, False) 
			direct_tmp_out = SetupTransmissionData(direct_sample, '1,2', backmon_start, backmon_end, wavbin, False)
			CalculateTransmission(trans_tmp_out,direct_tmp_out,trans_ws, trans_udet_mon, trans_udet_det, trans_wav1, trans_wav2)
					 
		mantid.deleteWorkspace(trans_tmp_out)
		mantid.deleteWorkspace(direct_tmp_out)
		# Apply the correction
		Divide(tmpWS, trans_ws, tmpWS)
	################################################################################     
	
	############################ Efficiency correction ##############################
	CorrectToFile(tmpWS, direct_beam_file, tmpWS, "Wavelength", "Divide")
	#################################################################################
	
	############################ Correct for sample/can volume ######################
	SANSUtility.ScaleByVolume(tmpWS, rescale)
	#################################################################################

	################################ Correction in Q space ##########################
	# 1D
	if correction_type == '1D':
		# Convert to Q
		ConvertUnits(tmpWS,tmpWS,"MomentumTransfer")
		ConvertUnits(resultWS,resultWS,"MomentumTransfer")
		
		# Need to mark the workspace as a distribution at this point to get next rebin right
		ws = mantid.getMatrixWorkspace(tmpWS)
		ws.isDistribution(True)
		
		# Calculate the solid angle corrections
		SolidAngle(tmpWS,"solidangle")

		# Rebin to desired Q bins
		q_bins = str(q1) + "," + str(dq) + "," + str(q2)
		Rebin(resultWS, resultWS, q_bins)
		Rebin(tmpWS, tmpWS, q_bins)
		RebinPreserveValue("solidangle", "solidangle", q_bins)

		# Sum all spectra
		SumSpectra(resultWS,resultWS)
		SumSpectra(tmpWS,tmpWS)
		SumSpectra("solidangle","solidangle")
	
		# Correct for solidangle
		Divide(tmpWS,"solidangle",tmpWS)
		mantid.deleteWorkspace("solidangle")

		# Now put back the fractional error from the raw count workspace into the result
		PoissonErrors(tmpWS, resultWS, resultWS)
	# 2D	
	else:
		# Run 2D algorithm !TASK! - saving stuff
		Qxy(tmpWS, resultWS, qxy2, dqxy)

	# Replaces NANs with zeroes but would be nice to have StripLRSpecialValues type routine	
	ReplaceSpecialValues(InputWorkspace=resultWS,OutputWorkspace=resultWS,NaNValue="0",InfinityValue="0")
	mantid.deleteWorkspace(tmpWS)
	
	return
############################# End of Correct function ##################################

############################# Script begins here  ######################################

### Sample correction #### 
# Put the components in the correct place
# 1. The sample
MoveInstrumentComponent(scatter_sample, 'some-sample-holder', Z = sample_z_corr, RelativePosition="1")

if instr_name == 'LOQ':
	xshift = (317.5 - xbeam_maskfile)/1000.
	yshift = (317.5 - ybeam_maskfile)/1000.
	MoveInstrumentComponent(scatter_sample, detbank, X = xshift, Y = yshift)
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
	#
	# MASK file stuff ==========================================================
	# correction terms to SANS2d encoders - store in MASK file ?
	Rear_Det_z_corr = 58.
	Rear_Det_x_corr = 0.0
	Front_Det_z_corr = 47.
	Front_Det_x_corr = -33.
	Front_Det_y_corr = -20.
	Front_Det_Rot_corr = 0.0
	
	rotateDet = (-Front_Det_Rot -  Front_Det_Rot_corr)
	RotateInstrumentComponent(scatter_sample,detbank,X="0.",Y="1.0",Z="0.",Angle=rotateDet)
	
	RotRadians = math.pi*(Front_Det_Rot + Front_Det_Rot_corr)/180.
	xshift = (Rear_Det_x + Rear_Det_x_corr - Front_Det_x - Front_Det_x_corr + Front_Det_Radius*sin(RotRadians ) )/1000. - front_det_default_x_m -xbeam_maskfile
	yshift = (Front_Det_y_corr /1000.  - ybeam_maskfile)
	# default in instrument description is 23.281m - 4.000m from sample at 19,281m !
	# need to add ~58mm to det1 to get to centre of detector, before it is rotated.
	zshift = (Front_Det_z + Front_Det_z_corr + Front_Det_Radius*(1 - cos(RotRadians )) )/1000.  -front_det_default_sd_m
	MoveInstrumentComponent(scatter_sample, 'front-detector', X = xshift, Y = yshift, Z = zshift, RelativePosition="1")

# For SANS2D runs before 568 the spectrum numbers were swapped around so correct for this if necessary
if instr_name == 'SANS2D':
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
	

# Final workspace is named run+{bank}+_correctiontype
final_workspace = scatter_sample.split('_')[0]
if detbank == 'front-detector':
	final_workspace += 'front'
elif detbank == 'HAB':
	final_workspace += 'hab'
elif detbank == 'main-detector-bank':
	final_workspace += 'main'
else:
	final_workspace += 'front'
	
final_workspace += '_' + correction_type
Correct(scatter_sample, trans_sample, final_workspace, "sample")

#if scatter_can:
#        Correct(scatter_can, trans_can, "tmp_can_output", "can")
#        Minus(final_workspace, "tmp_can_output", final_workspace)
#       mantid.deleteWorkspace("tmp_can_output")
