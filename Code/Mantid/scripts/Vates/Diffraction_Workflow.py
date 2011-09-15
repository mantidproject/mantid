# Basic parameters
filename = "TOPAZ_3131_event.nxs"
#Name of the workspaces to create
ws = "TOPAZ_3131"

# Load the original data
LoadEventNexus(Filename=filename,OutputWorkspace=ws)

# Convert to reciprocal space, in the sample frame
MakeDiffractionMDEventWorkspace(InputWorkspace=ws,OutputWorkspace=ws+'_MD',
		OutputDimensions='Q (sample frame)',LorentzCorrection='1')
		
# Find peaks
FindPeaksMD(InputWorkspace=ws+'_MD',MaxPeaks='50',OutputWorkspace=ws+'_peaks')

# Find the UB matrix using the peaks and known lattice parameters
FindUBUsingLatticeParameters(PeaksWorkspace=ws+'_peaks',a='10.3522',b='6.0768',c='4.7276',
		alpha='90',beta='90',gamma='90', NumInitial='20', Tolerance='0.12')
		
# Centroid the peaks
CentroidPeaksMD(InputWorkspace=ws+'_MD',CoordinatesToUse='Q (sample frame)',
		PeakRadius='0.05',PeaksWorkspace=ws+'_peaks',OutputWorkspace=ws+'_peaks_centered')
		
# Find UB again using the centroided-peaks. Should give about the same result
FindUBUsingLatticeParameters(PeaksWorkspace=ws+'_peaks_centered',a='10.3522',b='6.0768',c='4.7276',
		alpha='90',beta='90',gamma='90', NumInitial='20', Tolerance='0.12')
		
# Copy the UB matrix back to the original workspace
CopySample(InputWorkspace=ws+'_peaks_centered',OutputWorkspace=ws,
		CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0',  CopyLattice=1)
		
# Re-convert from eventWorkspace to MDWorkspace, this time in HKL space
MakeDiffractionMDEventWorkspace(InputWorkspace=ws,OutputWorkspace=ws+'_MD_hkl',OutputDimensions='HKL',LorentzCorrection='1')

# Save the MD workspace in Q sample frame to a NXS file. 
SaveMD(InputWorkspace=ws+'_MD', Filename='TOPAZ_3131_MD_q_sample.nxs')

# Save to a NXS file. Simultaneously makes the workspace file-backed (releasing some memory).
SaveMD(InputWorkspace=ws+'_MD_hkl', Filename='TOPAZ_3131_MD_hkl.nxs', MakeFileBacked=1)