LoadMD(Filename='SEQ_MDEW.nxs', OutputWorkspace='SEQ')
# Rebinned 4D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='[Qh,0,0],0,0.5,20', AlignedDim1='[0.5Qh,0.866Qk,0],-0.75,-0.25,20', AlignedDim2='[0,0,Ql],-0.9,-0.55,20', AlignedDim3='DeltaE,17,22,20', OutputWorkspace='SEQ_4D_rebin')
# Rebinned 4D->3D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='[Qh,0,0],0,0.5,20', AlignedDim1='DeltaE,17,22,20', AlignedDim2='[0.5Qh,0.866Qk,0],-0.75,-0.25,20', OutputWorkspace='SEQ_3D_rebin')
# Rebinned 4D->2D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='[Qh,0,0],0,0.5,20', AlignedDim1='DeltaE,17,22,20', OutputWorkspace='SEQ_2D_rebin')
# Rebinned 4D->1D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='DeltaE,17,22,20', OutputWorkspace='SEQ_1D_rebin')
# Integrated 4D->3D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='[Qh,0,0],0,0.5,20', AlignedDim1='DeltaE,17,22,20', AlignedDim2='[0.5Qh,0.866Qk,0],-0.75,-0.25,20', AlignedDim3='[0,0,Ql],-0.9,-0.55,1', OutputWorkspace='SEQ_3D_int')
# Integrated 4D->2D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='[Qh,0,0],0,0.5,20', AlignedDim1='DeltaE,17,22,20', AlignedDim2='[0.5Qh,0.866Qk,0],-0.75,-0.25,1', AlignedDim3='[0,0,Ql],-0.9,-0.55,1', OutputWorkspace='SEQ_2D_int')
# Integrated 4D->1D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='DeltaE,17,22,20', AlignedDim1='[Qh,0,0],0,0.5,1', AlignedDim2='[0.5Qh,0.866Qk,0],-0.75,-0.25,1', AlignedDim3='[0,0,Ql],-0.9,-0.55,1', OutputWorkspace='SEQ_1D_int')
