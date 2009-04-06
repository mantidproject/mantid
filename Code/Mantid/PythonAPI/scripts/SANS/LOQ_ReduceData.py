###########################################################################
# This is the template analysis script for reducing the LOQ data.         #
# It has place holders so that the appropriate data can be filled in at   #
# run-time by the SANS GUI                                                #
#                                                                         #
#  Authors: Russell Taylor, Tessella plc (Original script)                #
#           Martyn Gigg, Tessella plc (Adapted to GUI use)                #
#                                                                         #                    
###########################################################################
import LOQFunctions

# Sort out the input. The tags get replaced by information in the LOQ GUI
scatter_sample = '<SCATTERSAMPLE>'
scatter_can = '<SCATTERCAN>'
trans_sample = '<TRANSMISSIONSAMPLE>'
direct_sample = '<DIRECTSAMPLE>'
maskstring = '<MASKSTRING>'
instr_dir = '<INSTRUMENTPATH>'
rmin = <RADIUSMIN>/1000.0
rmax = <RADIUSMAX>/1000.0
xshift = str( (317.5 - <XBEAM>)/1000.0 )
yshift = str( (317.5 - <YBEAM>)/1000.0 )
wav1 = <WAVMIN>
wav2 = <WAVMAX>
dwav = <WAVDELTA>
q1 = <QMIN>
q2 = <QMAX>
dq = <QDELTA>
rescale = <SCALEFACTOR>*100.0

direct_beam_file = '<DIRECTFILE>'

### Main correction function ###
def Correct(inputWS):
	'''Performs the data reduction steps'''
	monitorWS = "Monitor-" + inputWS
	wavname = "_" + str(int(wav1)) + "_" + str(int(wav2))
	resultWS = "SmallAngle-" + inputWS + wavname
	# Get the monitor
	LOQFunctions.GetMonitor(inputWS, monitorWS)
	# Get small angle banks
	firstsmall = 130
	lastsmall = 16130
	# Get the small angle banks
	LOQFunctions.GetMainBank(inputWS, firstsmall, lastsmall, resultWS)
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
	
	# Step 4 - Correct by incident beam monitor
	# At this point need to fork off workspace name to keep a workspace containing raw counts
	# outputWS_cor = outputWS + "_tmp"
	tmpWS = "temporary_workspace"
	Divide(resultWS, monitorWS, tmpWS)

 	# Step 6 - Correct by transmission
 	# Need to remove prompt spike and, later, flat background
	trans_tmp_out = LOQFunctions.SetupTransmissionData(trans_sample, instr_dir + "/LOQ_trans_Definition.xml", wavbin)
	direct_tmp_out = LOQFunctions.SetupTransmissionData(direct_sample, instr_dir + "/LOQ_trans_Definition.xml", wavbin)
	trans_ws = "transmission-" + inputWS + wavname
	CalculateTransmission(trans_tmp_out,direct_tmp_out,trans_ws)
	mantid.deleteWorkspace(trans_tmp_out)
	mantid.deleteWorkspace(direct_tmp_out)
	# Apply the correction
	Divide(tmpWS, trans_ws, tmpWS)
	
	# Step 7 - Correct for efficiency
	CorrectToFile(tmpWS, direct_beam_file, tmpWS, "Wavelength", "Divide")

	# Step 11 - Convert to Q
	# Convert units to Q (MomentumTransfer)
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
	mantid.deleteWorkspace(tmpWS)

	# Correct for sample/Can volume
	LOQFunctions.ScaleByVolume(resultWS, rescale);

	return resultWS

### End of Correct function ###

# Cross section (remove can scattering)
# Perform correction for the sample and can.
# result = sample - can

# Final workspace containing the output of the sample correction
sample_correction = Correct(scatter_sample)
can_correction = Correct(scatter_can)

Minus(sample_correction, can_correction, sample_correction + "-corrected")
#######################


#######################
#step 12 - Save 1D data
# this writes to the /bin directory - why ?
# Want to be able to write out to new XML format
#SaveRKH(resultWS+wavname,Filename=resultWS+wavname+".Q",FirstColumnValue="MomentumTransfer")

