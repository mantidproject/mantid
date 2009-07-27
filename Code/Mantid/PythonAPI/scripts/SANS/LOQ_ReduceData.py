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
# First the worksapces
scatter_sample = '|SCATTERSAMPLE|'
scatter_can = '|SCATTERCAN|'
trans_sample = '|TRANSMISSIONSAMPLE|'
trans_can = '|TRANSMISSIONCAN|'
direct_sample = '|DIRECTSAMPLE|'

# Now the mask string
maskstring = '|MASKSTRING|'

# The instrument dependent information
instr_dir = '|INSTRUMENTPATH|'
instr_name = '|INSTRUMENTNAME|'
specmin = |SPECMIN|
specmax = |SPECMAX|
xcentre = |XBEAM|/1000.
ycentre = |YBEAM|/1000.
# Which bank are we looking at
detbank = ''
xshift = str( (317.5/1000.0) - xcentre)
yshift = str( (317.5/1000.0) - ycentre)

if instr_name == 'LOQ':
	if specmin < 16387:
		detbank = 'main-detector-bank'
	else:
		detbank = 'HAB'	
	xshift = str( (317.5/1000.0) - xcentre)
	yshift = str( (317.5/1000.0) - ycentre)
else:
	if specmin < 36865:
		detbank = 'front-detector'
	else:
		detbank = 'rear-detector'
	xshift = str( (484.4/1000.0) - xcentre)
	yshift = str( (337.4/1000.0) - ycentre)


dimension = int(sqrt(specmax - specmin + 1))
monitorspectrum = |MONSPEC|
rmin = |RADIUSMIN|/1000.0
rmax = |RADIUSMAX|/1000.0

# Analysis values
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

fitmon_start = '19900'
fitmon_end = ' 20500'
backmon_start = '31000'
backmon_end = ' 39000'
 
# This indicates whether a 1D or a 2D analysis is performed
correction_type = '|ANALYSISTYPE|'

### Main correction function ###
def Correct(sampleWS, transWS, resultWS, suffix):
	'''Performs the data reduction steps'''
	monitorWS = "Monitor"
	# Get the monitor ( StartSpectrum is off by one with cropworkspace)
	CropWorkspace(sampleWS, OutputWorkspace = monitorWS, StartSpectrum=str(monitorspectrum - 1), EndSpectrum=str(monitorspectrum - 1))
	RemoveBins(monitorWS, monitorWS, fitmon_start, fitmon_end,Interpolation="Linear")
	FlatBackground(monitorWS, monitorWS, '0', backmon_start, backmon_end)

	# Get the bank we are looking at
	CropWorkspace(sampleWS, resultWS, StartSpectrum=str(specmin - 1),EndSpectrum=str(specmax - 1))

	# Mask the corners and beam stop if radius parameters are given
	if rmin > 0.0: 
		LOQFunctions.MaskInsideCylinder(resultWS, rmin)
	if rmax > 0.0:
		LOQFunctions.MaskOutsideCylinder(resultWS, rmax)
	
	# Move the instrument to the correct place
	MoveInstrumentComponent(resultWS, detbank, X = xshift, Y = yshift, RelativePosition="1")

	# Mask others that are defined
	speclist = LOQFunctions.ConvertToSpecList(maskstring, specmin, dimension)
	LOQFunctions.MaskBySpecNumber(resultWS, speclist)


	# Convert all of the files to wavelength and rebin
	# ConvertUnits does have a rebin option, but it's crude. In particular it rebins on linear scale.
	wavbin = str(wav1) + "," + str(dwav) + "," + str(wav2)

	ConvertUnits(monitorWS, monitorWS, "Wavelength")
	Rebin(monitorWS, monitorWS,wavbin)
	ConvertUnits(resultWS,resultWS,"Wavelength")
	Rebin(resultWS,resultWS,wavbin)
	      
	# Correct by incident beam monitor
	# At this point need to fork off workspace name to keep a workspace containing raw counts
	tmpWS = "temporary_workspace"
	Divide(resultWS, monitorWS, tmpWS)
	mantid.deleteWorkspace(monitorWS)

 	# Correct by transmission
 	# Need to remove prompt spike and, later, flat background
 	if instr_name == 'LOQ' and transWS and direct_sample:
                # Change the instrument definition to the correct one in the LOQ case 
                LoadInstrument(transWS, instr_dir + "/LOQ_trans_Definition.xml")
                LoadInstrument(direct_sample, instr_dir + "/LOQ_trans_Definition.xml")
                trans_tmp_out = LOQFunctions.SetupTransmissionData(transWS, '1,2', wavbin)
        	direct_tmp_out = LOQFunctions.SetupTransmissionData(direct_sample, '1,2', wavbin)
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
		
	else:
		# Run 2D algorithm
		Qxy(tmpWS, resultWS, str(qxy2), str(dqxy))
		
	mantid.deleteWorkspace(tmpWS)
	return

### End of Correct function ###

# Cross section (remove can scattering)
# Perform correction for the sample and can.
# result = sample - can
# Final workspace containing the output of the sample correction
final_workspace = scatter_sample.split('_')[0] + '_' + correction_type
Correct(scatter_sample, trans_sample, final_workspace, "sample")
if scatter_can:
        Correct(scatter_can, trans_can, "tmp_can_output", "can")
        Minus(final_workspace, "tmp_can_output", final_workspace)
        mantid.deleteWorkspace("tmp_can_output")
