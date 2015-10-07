LoadMD(Filename='SEQ_MDEW.nxs', OutputWorkspace='SEQ')
# Rebinned 4D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='[Qh,0,0],0,0.5,20', AlignedDim1='[0.5Qh,0.866Qk,0],-0.75,-0.25,20', AlignedDim2='[0,0,Ql],-0.9,-0.55,20', AlignedDim3='DeltaE,17,22,20', OutputWorkspace='SEQ_4D_rebin')
# Rebinned 4D->3D MDHistoWorkspace
BinMD(InputWorkspace='SEQ', AlignedDim0='[Qh,0,0],0,0.5,20', AlignedDim1='DeltaE,17,22,20', AlignedDim2='[0.5Qh,0.866Qk,0],-0.75,-0.25,20', OutputWorkspace='SEQ_3D_rebin')
BinMD(InputWorkspace='SEQ', AlignedDim0='[Qh,0,0],0,0.5,20', AlignedDim1='DeltaE,17,22,20', AlignedDim2='[0.5Qh,0.866Qk,0],-0.75,-0.25,20', AlignedDim3='[0,0,Ql],-0.9,-0.55,1', OutputWorkspace='SEQ_3D_int')
