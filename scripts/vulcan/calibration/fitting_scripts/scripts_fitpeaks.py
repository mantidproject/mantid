
# Fit 1 peak for focused bank 1, 2 and 3 (3 banks)
input_ws_name = 'VULCAN_164960_CC_Masked'
peakparnames = 'I, A, B, X0, S'
peakparvalues = '102176394 , 1200 , 800 , 1.07 , 0.00255'
FitPeaks(InputWorkspace=input_ws_name,
         StartWorkspaceIndex=0,
         StopWorkspaceIndex=5,
         PeakFunction="BackToBackExponential", BackgroundType="Linear",
         PeakCenters="1.0758",
         FitWindowBoundaryList="1.05, 1.15",
         PeakParameterNames=peakparnames, PeakParameterValues=peakparvalues,
         FitFromRight=True,
         HighBackground=False,
         OutputWorkspace="Bank1_1Peak",
         OutputPeakParametersWorkspace="PeakParameters_Bank1_1Peak",
         FittedPeaksWorkspace="Bank1_1Peak")
