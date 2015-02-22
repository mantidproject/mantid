#pylint: disable=invalid-name
# Basic parameters  for  Triphylite Crystal
#Name of the workspaces to create
ws_name = "TOPAZ_3132"
filename = ws_name +"_event.nxs"
ws = LoadEventNexus(Filename=filename,FilterByTofMin=3000, FilterByTofMax=16000)

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Part 1. Basic Reduction

# Spherical Absorption and Lorentz Corrections
ws = AnvredCorrection(InputWorkspace=ws, LinearScatteringCoef=0.451, LinearAbsorptionCoef=0.993, Radius=0.14)

# Convert to Q space
LabQ = ConvertToDiffractionMDWorkspace(InputWorkspace=ws, LorentzCorrection='0',\
        OutputDimensions='Q (lab frame)', SplitInto=2, SplitThreshold=150)

# Find peaks
PeaksLattice = FindPeaksMD(InputWorkspace=LabQ,MaxPeaks=100)

# 3d integration to centroid peaks
PeaksLattice = CentroidPeaksMD(InputWorkspace=LabQ,\
    PeakRadius=0.12, PeaksWorkspace=PeaksLattice)

# Find the UB matrix using the peaks and known lattice parameters
FindUBUsingLatticeParameters(PeaksWorkspace=PeaksLattice, a=10.3522, b=6.0768, c=4.7276,\
                alpha=90, beta=90, gamma=90, NumInitial=20, Tolerance=0.12)

# And index to HKL
IndexPeaks(PeaksWorkspace=PeaksLattice, Tolerance=0.12)

# Integrate peaks in Q space using spheres
PeaksLattice_Integrated = IntegratePeaksMD(InputWorkspace=LabQ,PeakRadius=0.12,\
    BackgroundOuterRadius=0.18,BackgroundInnerRadius=0.15,\
    PeaksWorkspace=PeaksLattice)

# Save for SHELX
SaveHKL(InputWorkspace=PeaksLattice, Filename=ws_name + '.hkl')

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Part 2. Alternative/Advanced Processing Steps


# Find peaks again for FFT
PeaksLatticeFFT = FindPeaksMD(InputWorkspace=LabQ, MaxPeaks=100)

# 3d integration to centroid peaks
PeaksLatticeFFT = CentroidPeaksMD(InputWorkspace=LabQ,\
    PeakRadius=0.12, PeaksWorkspace=PeaksLatticeFFT)

# Find the UB matrix using FFT
FindUBUsingFFT(PeaksWorkspace=PeaksLatticeFFT, MinD=3.0, MaxD=14.0)

# And index to HKL
IndexPeaks(PeaksWorkspace=PeaksLatticeFFT, Tolerance=0.12)

# Integrate peaks in Q space using spheres
PeaksLatticeFFT = IntegratePeaksMD(InputWorkspace=LabQ, PeakRadius=0.12,\
    BackgroundOuterRadius=0.18,BackgroundInnerRadius=0.15,\
    PeaksWorkspace=PeaksLatticeFFT)

# Save for SHELX
SaveHKL(InputWorkspace=PeaksLatticeFFT, Filename=ws_name + '.hkl')

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Part 3. Utilising the UB

# Copy the UB matrix back to the original workspace
CopySample(InputWorkspace=PeaksLattice, OutputWorkspace=ws,\
    	CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0',  CopyLattice=1)

# Convert to reciprocal space, in the sample frame
HKL = ConvertToDiffractionMDWorkspace(InputWorkspace=ws,\
    	OutputDimensions='HKL',LorentzCorrection='0', SplitInto='2',SplitThreshold='150')

# -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# Part 4. Displaying

# Bin to a regular grid
Binned = BinMD(InputWorkspace=HKL,AlignedDim0='[H,0,0], -15, 5, 150',AlignedDim1='[0,K,0], -0, 10, 50',AlignedDim2='[0,0,L], 0, 12,  150')

# Show in slice Viewer
sv = plotSlice(Binned, xydim=('[H,0,0]','[0,0,L]'), slicepoint=[0, +9, 0], colorscalelog=True)
sv.setColorMapBackground(0,0,0)
