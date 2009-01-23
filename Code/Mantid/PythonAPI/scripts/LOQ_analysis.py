#LOQ data analysis script
#First draft
#######################
#Step 1 - Load the data file 
LoadDataAlg = LoadRawDialog(OutputWorkspace="Monitors",spectrummin="1",spectrummax="2")
data_file = LoadDataAlg.getPropertyValue("Filename")
#Load the small angle bank
LoadRaw(Filename = data_file, OutputWorkspace="Small_Angle",spectrummin="3",spectrummax="16386")
#Load the high angle bank
LoadRaw(Filename = data_file, OutputWorkspace="High_Angle",spectrummin="16386")

#######################
#Step 1.5 - Mask out unwanted detetctors
MarkDeadDetectors("Small_Angle",DetectorList="200,201,202,203,204,205")
#Future Missing - Mask by volume projection

#######################
#Step 2 - Convert all of the files to wavelength (including rebin)
ConvertUnits("Small_Angle","Small_Angle","Wavelength",AlignBins="Yes")

#######################
#step 2.5 remove unwanted Wavelength bins
#Missing-remove non edge bins and linear interpolate #373
RemoveBins("Small_Angle","1.05", "1.20")
#Future Missing - add cubic interpolation

#######################
#Step 3 - Flat cell correction

#OPTION 1
#Load flood Source file (data only for the Small angle bank)
LoadRawDialog(OutputWorkspace="Flood_Source_Small_Angle",spectrummin="3",spectrummax="16386")
#Convert to wavelength
#Future - correct for ratio of solid angles for different positions
#Rebin to "Small_Angle" workspace
#?? Normalise the flood source file
Divide("Small_Angle","Flood_Source_Small_Angle","Small_Angle_norm_flood_source")

#OPTION 2
CorrectToFile("Small_Angle","C:/path_to_file/file.001","Small_Angle","Divide")
mtdHelp("CorrectToFile")
#LoadRKH with scalar value for wavelength ranges
#data/flat(wavelength)

#######################
#Step 4 - Correct by incident beam monitor
#CropMonitorsWorkspace to extract incident beam monitor in a seperate workspace
#Missing - RemoveBackground
#??Normalise?
#Rebin To workspace
#divide

#######################
#Step 6 - Correct by transmission
#Missing CorrectByTransmission algorithm to do the following #375
	#Load transmission data
	#Load direct data
	#handle conversion of both to instrument transmission mode
	#convert to wavelength and rebin
	# transM3/transM2 = trans
	# directM3/directM2 = direct
	# correction = trans/direct
	#rebin correction to data
	# fit linear to log(correction)
	# data/fit

#######################
#Step 7 - Correct for efficiency
#LoadRKH with scalar value for wavelength ranges #370
#data/efficiency(wavelength)

#######################
#Step 8 - Rescale(detector)
#LoadRKH with scalar value for each detector (as 1-1 also spectra) #370
#data*Rescale(detector)

#######################
#Step 10 - Correct for sample/Can
#Missing Algorithm - Input sample/can values, perform division by single value

#######################
#Step 11 - Convert to Q
#Convert units to Q (MomentumTransfer??)
#rebin to desired Q bins
#Missing - Sum all spectra #376 DONE

#######################
#step 12 - Cross section (remove can scatering)
#Perform steps 1-11 for the sample and can
# sample-can

#######################
#step 12 - Save 1D data
#Missing - save as RKH format #377
