# Basic parameters
filename = "TOPAZ_3131_event.nxs"
#Name of the workspaces to create
ws = "TOPAZ_3131"

# Load the original data
LoadEventNexus(Filename=filename,OutputWorkspace=ws)

# Convert to reciprocal space, in the sample frame
ConvertToDiffractionMDWorkspace(InputWorkspace=ws,OutputWorkspace=ws+'_MD',
		OutputDimensions='Q (sample frame)',LorentzCorrection='1')
		
# Find peaks
FindPeaksMD(InputWorkspace=ws+'_MD',MaxPeaks='50',OutputWorkspace=ws+'_peaks')

# Find the UB matrix using the peaks and known lattice parameters
FindUBUsingLatticeParameters(PeaksWorkspace=ws+'_peaks',a='10.3522',b='6.0768',c='4.7276',
		alpha='90',beta='90',gamma='90', NumInitial='20', Tolerance='0.12')
# And index to HKL		
IndexPeaks(PeaksWorkspace=ws+'_peaks', Tolerance='0.12')
		
# Centroid the peaks
CentroidPeaksMD(InputWorkspace=ws+'_MD',CoordinatesToUse='Q (sample frame)',
		PeakRadius='0.05',PeaksWorkspace=ws+'_peaks',OutputWorkspace=ws+'_peaks_centered')
		
# Find UB again using the centroided-peaks. Should give about the same result
FindUBUsingLatticeParameters(PeaksWorkspace=ws+'_peaks_centered',a='10.3522',b='6.0768',c='4.7276',
		alpha='90',beta='90',gamma='90', NumInitial='20', Tolerance='0.12')
# And index to HKL		
IndexPeaks(PeaksWorkspace=ws+'_peaks_centered', Tolerance='0.12')

# Copy the UB matrix back to the original workspace
CopySample(InputWorkspace=ws+'_peaks_centered',OutputWorkspace=ws,
		CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0',  CopyLattice=1)
		
# Integrate in reciprocal space 
IntegratePeaksMD(InputWorkspace=ws+'_MD', CoordinatesToUse='Q (sample frame)', PeakRadius=0.1, BackgroundRadius=0.0, \
				PeaksWorkspace=ws+'_peaks_centered', OutputWorkspace=ws+'_peaks_centered_integ')

# Integrate the non-centroided ones
IntegratePeaksMD(InputWorkspace=ws+'_MD', CoordinatesToUse='Q (sample frame)', PeakRadius=0.1, BackgroundRadius=0.0, \
				PeaksWorkspace=ws+'_peaks', OutputWorkspace=ws+'_peaks_integ')

# Save the MD workspace in Q sample frame to a NXS file. 
SaveMD(InputWorkspace=ws+'_MD', Filename='TOPAZ_3131_MD_q_sample.nxs')

# ----- Now Convert to HKL space to view in that space ----------- 

# Convert to reciprocal space, in the sample frame
ConvertToDiffractionMDWorkspace(InputWorkspace=ws,OutputWorkspace=ws+'_HKL',
		OutputDimensions='HKL',LorentzCorrection='1')

# Save that for later viewing in paraview
SaveMD(InputWorkspace=ws+'_HKL', Filename='TOPAZ_3131_MD_HKL.nxs')


		
# ------ Now predict peaks and integrate those -----------
if True:
	PredictPeaks(InputWorkspace=ws, MinDSpacing=0.75, OutputWorkspace=ws+'_peaks_predicted')
	# Does not work quite well, see ticket #3772
	IntegratePeaksMD(InputWorkspace=ws+'_MD', CoordinatesToUse='Q (sample frame)', PeakRadius=0.1, BackgroundRadius=0.0, \
					PeaksWorkspace=ws+'_peaks_predicted', OutputWorkspace=ws+'_peaks_predicted_integ')
					
	BinMD(InputWorkspace='TOPAZ_3131_HKL',AlignedDimX='H,-20,0,200',AlignedDimY='K,-5,15,200',AlignedDimZ='L,-5,15,200',OutputWorkspace='hkl')
	
	BinMD(InputWorkspace='TOPAZ_3131_HKL',AlignedDimX='H,-20,0,500',AlignedDimY='K,-5,15,500',AlignedDimZ='L,-5,15,100',OutputWorkspace='hkl_2')
	
	
