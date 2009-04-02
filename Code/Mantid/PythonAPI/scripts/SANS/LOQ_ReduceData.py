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
scatter_can = '<SCATTERSAMPLE>'
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
def Correct(inputWS, outputWS):
	'''Performs the data reduction steps '''
	firstsmall = 130
	lastsmall = 16130
	LOQFunctions.SetupSmallAngle(inputWS, outputWS, firstsmall, lastsmall, rmin, rmax, maskstring, xshift, yshift)

	# Convert all of the files to wavelength and rebin
	# ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
	wavbin = str(wav1) + "," + str(dwav) + "," + str(wav2)
	wavname = "_" + str(int(wav1)) + "_" + str(int(wav2))

	ConvertUnits("Monitor","Monitor","Wavelength")
	Rebin("Monitor","Monitor",wavbin)
	ConvertUnits(outputWS,outputWS,"Wavelength")
	Rebin(outputWS,outputWS,wavbin)

# ConvertUnits("High_Angle","High_Angle","Wavelength")
# Rebin("High_Angle","High_Angle",wavbin)
	
# FROM NOW ON JUST LOOKING AT MAIN DETECTOR DATA.....
	
# step 2.5 remove unwanted Wavelength bins
# Remove non edge bins and linear interpolate #373
# RemoveBins("High_Angle","High_Angle","3.00","3.50",Interpolation="Linear")
# Future Missing - add cubic interpolation


# Step 3 - Flat cell correction

# OPTION 1
# calculate from raw flood source file
# maybe later!

# OPTION 2
# LoadRKH with scalar value for wavelength ranges
# data/flat(wavelength)
# LoadRKH(path+"Data/LOQ sans configuration/FLAT_CELL.061","flat","SpectraNumber")
# CropWorkspace("flat","flat",StartSpectrum=str(firstsmall),EndSpectrum=str(lastsmall))
# optional correct for diffrernt detector positions
# SolidAngle("flat","flatsolidangle")
# SolidAngle(outputWS,"solidangle")
# Divide("flatsolidangle","solidangle","SAcorrection")
# Divide("flat","SAcorrection","flat")
# RETRY
# Divide(outputWS,"flat",outputWS)

# OPTION 3
# Correction using instrument geometry
# SolidAngle(outputWS,"solidangle")
# Divide(outputWS,"solidangle","Small_Angle corrected by solidangle")

	# Step 4 - Correct by incident beam monitor
	# At this point need to fork off workspace name to keep a workspace containing raw counts
	outputWS_cor = outputWS + "_tmp"
	Divide(outputWS, "Monitor",outputWS_cor)

# 	# Step 6 - Correct by transmission
# 	# Need to remove prompt spike and, later, flat background
	trans_tmp_out = LOQFunctions.SetupTransmissionData(trans_sample, instr_dir + "/LOQ_trans_Definition.xml", wavbin)
	direct_tmp_out = LOQFunctions.SetupTransmissionData(direct_sample, instr_dir + "/LOQ_trans_Definition.xml", wavbin)
	trans_ws = "transmission" + wavname
	CalculateTransmission(trans_tmp_out,direct_tmp_out,trans_ws)
	mantid.deleteWorkspace(trans_tmp_out)
	mantid.deleteWorkspace(direct_tmp_out)
	# Apply the correction
	Divide(outputWS_cor, trans_ws, outputWS_cor)
	
	# Step 7 - Correct for efficiency
	CorrectToFile(outputWS_cor, direct_beam_file, outputWS_cor, "Wavelength", "Divide")

	# Step 11 - Convert to Q
	# Convert units to Q (MomentumTransfer)
	ConvertUnits(outputWS_cor,outputWS_cor,"MomentumTransfer")
	ConvertUnits(outputWS,outputWS,"MomentumTransfer")
	
	# Need to mark the workspace as a distribution at this point to get next rebin right
	ws = mantid.getMatrixWorkspace(outputWS_cor)
	ws.isDistribution(True)

	# Calculate the solid angle corrections
	SolidAngle(outputWS_cor,"solidangle")

	# Rebin to desired Q bins
	q_bins = str(q1) + "," + str(dq) + "," + str(q2)
	Rebin(outputWS,outputWS,q_bins)
	Rebin(outputWS_cor,outputWS_cor,q_bins)
	RebinPreserveValue("solidangle","solidangle",q_bins)

	# Sum all spectra
	SumSpectra(outputWS,outputWS)
	SumSpectra(outputWS_cor,outputWS_cor)
	SumSpectra("solidangle","solidangle")
	
	# Correct for solidangle
	Divide(outputWS_cor,"solidangle",outputWS_cor)
	mantid.deleteWorkspace("solidangle")

	# Now put back the fractional error from the raw count workspace into the result
	PoissonErrors(outputWS_cor, outputWS, outputWS + wavname)
	mantid.deleteWorkspace(outputWS_cor)
	mantid.deleteWorkspace(outputWS)

	# Correct for sample/Can volume
	LOQFunctions.ScaleByVolume(outputWS + wavname, rescale);

	return outputWS + wavname

### End of Correct function ###

# Cross section (remove can scattering)
# Perform correction for the sample and can and result = sample - can

# Final workspace containing the output of the sample correction
sample_correction = Correct(scatter_sample, "Small_Angle_sample")
#can_correction = Correct(scatter_can, "Small_Angle_can")

#Minus(sample_correction, can_correction, sample_correction)

#######################


#######################
#step 12 - Save 1D data
# this writes to the /bin directory - why ?
#SaveRKH(outputWS+wavname,Filename=outputWS+wavname+".Q",FirstColumnValue="MomentumTransfer")

