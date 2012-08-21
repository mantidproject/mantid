"""
ISIS reflectometry reduction script.
"""

from mantid.simpleapi import *

class QuickReduction:

    #__input_workspace_name = ""
    
    def __init__(self, workspace_name):
        self.__input_workspace_name = workspace_name
    
    def execute(self):
        
        workspace_name = self.__input_workspace_name
        workspace_nexus_file = workspace_name + ".nxs"
        
        PIX=1.1E-3 #TODO. Extract from the IDF along with Start and EndWorkspace index etc.

        Load(Filename=workspace_nexus_file,OutputWorkspace=workspace_name)
        X=mtd[workspace_name]
        X = ConvertUnits(InputWorkspace=X,Target="Wavelength",AlignBins="1")
        # Reference intensity to normalise by
        CropWorkspace(InputWorkspace=X,OutputWorkspace='Io',XMin=0.8,XMax=14.5,StartWorkspaceIndex=2,EndWorkspaceIndex=2)
        # Crop out transmission and noisy data 
        CropWorkspace(InputWorkspace=X,OutputWorkspace='D',XMin=0.8,XMax=14.5,StartWorkspaceIndex=3)
        Io=mtd['Io']
        D=mtd['D']
    
        # Peform the normaisation step
        Divide(LHSWorkspace=D,RHSWorkspace=Io,OutputWorkspace='I',
               AllowDifferentNumberSpectra='1',ClearRHSWorkspace='1')
        I=mtd['I'][0]
        
        # Automatically determine the SC and averageDB 
        FindReflectometryLines(InputWorkspace=I, StartWavelength=10, OutputWorkspace='spectrum_numbers')
        spectrum_table = mtd['spectrum_numbers']
        SC = spectrum_table.cell(0, 0) #Spectrum number for the reflected line.
        avgDB = spectrum_table.cell(0, 1) #Spectrum number for the transmission line.
        
        # Move the detector so that the detector channel matching the reflected beam is at 0,0
        MoveInstrumentComponent(Workspace=I,ComponentName="lineardetector",X=0,Y=0,Z=-PIX*( (SC-avgDB)/2.0 +avgDB) )
        
        # Should now have signed theta vs Lambda
        ConvertSpectrumAxis(InputWorkspace=I,OutputWorkspace='SignedTheta_vs_Wavelength',Target='signed_theta')
  
        # MD transformations
        ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength',OutputWorkspace='QxQy',OutputDimensions='Q (lab frame)', Extents='-0.0005,0.0005,0,0.12')
        ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength',OutputWorkspace='KiKf',OutputDimensions='K (incident, final)', Extents='0,0.05,0,0.05')
        ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength',OutputWorkspace='PiPf',OutputDimensions='P (lab frame)', Extents='0,0.1,-0.02,0.15')
        
        # Bin the outputs to histograms because observations are not important.
        BinMD(InputWorkspace='QxQy',AxisAligned='0',BasisVector0='Qx,(Ang^-1),1,0',BasisVector1='Qz,(Ang^-1),0,1',OutputExtents='-0.0005,0.0005,0,0.12',OutputBins='100,100',Parallel='1',OutputWorkspace='QxQy_rebinned')
        BinMD(InputWorkspace='KiKf',AxisAligned='0',BasisVector0='Ki,(Ang^-1),1,0',BasisVector1='Kf,(Ang^-1),0,1',OutputExtents='0,0.05,0,0.05',OutputBins='200,200',Parallel='1',OutputWorkspace='KiKf_rebinned')
        BinMD(InputWorkspace='PiPf',AxisAligned='0',BasisVector0='Pz_i + Pz_f,(Ang^-1),1,0',BasisVector1='Pz_i - Pz_f,(Ang^-1),0,1',OutputExtents='0,0.1,-0.02,0.15',OutputBins='50,50',Parallel='1',OutputWorkspace='PiPf_rebinned')
       
        
        