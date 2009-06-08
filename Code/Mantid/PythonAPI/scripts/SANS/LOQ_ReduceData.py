###########################################################################
# This is the template analysis script for reducing the LOQ data.         #
# It has place holders so that the appropriate data can be filled in at   #
# run time by the SANS GUI                                                #
#                                                                         #
#  Authors: Russell Taylor, Tessella plc (Original script)                #
#           Martyn Gigg, Tessella plc (Adapted to GUI use)                #
#                                                                         #                    
###########################################################################
import LOQFunctions

# Sort out the input. The tags get replaced by information in the LOQ GUI
scatter_sample = '|SCATTERSAMPLE|'
scatter_can = '|SCATTERCAN|'
trans_sample = '|TRANSMISSIONSAMPLE|'
trans_can = '|TRANSMISSIONCAN|'
direct_sample = '|DIRECTSAMPLE|'
maskstring = '|MASKSTRING|'
instr_dir = '|INSTRUMENTPATH|'
rmin = |RADIUSMIN|/1000.0
rmax = |RADIUSMAX|/1000.0
xshift = str( (317.5 - |XBEAM|)/1000.0 )
yshift = str( (317.5 - |YBEAM|)/1000.0 )
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
CORRECTIONTYPE = '|ANALYSISTYPE|'

### Main correction function ###
def Correct(sampleWS, transWS, resultWS, suffix):
	'''Performs the data reduction steps'''
	monitorWS = "Monitor"
	# Get the monitor
	LOQFunctions.GetMonitor(sampleWS, monitorWS, 2)
	# Get small angle banks
	firstsmall = 130
	lastsmall = 16130
	# Get the small angle banks
	LOQFunctions.GetMainBank(sampleWS, firstsmall, lastsmall, resultWS)
	# Mask beam stop
	LOQFunctions.MaskInsideCylinder(resultWS, rmin)
	# Mask corners
	LOQFunctions.MaskOutsideCylinder(resultWS, rmax)
	# Mask others that are defined
	detlist = LOQFunctions.ConvertToDetList(maskstring)
	LOQFunctions.MaskByDetNumber(resultWS, detlist)
	MoveInstrumentComponent(resultWS, "main-detector-bank", X = xshift, Y = yshift, RelativePosition="1")

	# Convert all of the files to wavelength and rebin
	# ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
	wavbin = str(wav1) + "," + str(dwav) + "," + str(wav2)

	ConvertUnits(monitorWS, monitorWS, "Wavelength")
	Rebin(monitorWS, monitorWS,wavbin)
	ConvertUnits(resultWS,resultWS,"Wavelength")
	Rebin(resultWS,resultWS,wavbin)
	      
	# ConvertUnits("High_Angle","High_Angle","Wavelength")
	# Rebin("High_Angle","High_Angle",wavbin)
	
	# Correct by incident beam monitor
	# At this point need to fork off workspace name to keep a workspace containing raw counts
	tmpWS = "temporary_workspace"
	Divide(resultWS, monitorWS, tmpWS)
	mantid.deleteWorkspace(monitorWS)

 	# Correct by transmission
 	# Need to remove prompt spike and, later, flat background
 	if transWS and direct_sample:
                trans_tmp_out = LOQFunctions.SetupTransmissionData(transWS, instr_dir + "/LOQ_trans_Definition.xml", wavbin)
        	direct_tmp_out = LOQFunctions.SetupTransmissionData(direct_sample, instr_dir + "/LOQ_trans_Definition.xml", wavbin)
                trans_ws = transWS.split('_')[0] + "_trans_" + suffix
                CalculateTransmission(trans_tmp_out,direct_tmp_out,trans_ws)
                mantid.deleteWorkspace(trans_tmp_out)
                mantid.deleteWorkspace(direct_tmp_out)
                # Apply the correction
                Divide(tmpWS, trans_ws, tmpWS)
	
	# Correct for efficiency
	CorrectToFile(tmpWS, direct_beam_file, tmpWS, "Wavelength", "Divide")

	# Correct for sample/Can volume
	LOQFunctions.ScaleByVolume(tmpWS, rescale);

	# Steps differ here depending on type
	if CORRECTIONTYPE == '1D':
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
		
	else:
		# Run 2D algorithm
		Qxy(tmpWS, resultWS, "small_angle2D.Q", str(qxy2), str(dqxy))
		
	mantid.deleteWorkspace(tmpWS)
	return

### End of Correct function ###

# Cross section (remove can scattering)
# Perform correction for the sample and can.
# result = sample - can
# Final workspace containing the output of the sample correction
final_workspace = scatter_sample.split('_')[0] + '_' + CORRECTIONTYPE
Correct(scatter_sample, trans_sample, final_workspace, "sample")
if scatter_can:
        Correct(scatter_can, trans_can, "tmp_can_output", "can")
        Minus(final_workspace, "tmp_can_output", final_workspace)
        mantid.deleteWorkspace("tmp_can_output")
