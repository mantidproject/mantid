import sys
sys.path.append('/Applications/MantidPlot_312.app/Contents/MacOS')
from mantid.simpleapi import *

Load(Filename='/Users/keithbutler/Documents/Mantid/Data/Muons/Diffusion/EMU00011892.nxs', OutputWorkspace='EMU00011892')  # 20180607084524408319000
Load(Filename='/Users/keithbutler/Documents/Mantid/Data/Muons/Nickel/35566.NXS', OutputWorkspace='35566')  # 20180607084524470143000
Load(Filename='/Users/keithbutler/Documents/Mantid/Data/Muons/Cu/Cu_Training_Course_Data/EMU00020896.nxs', OutputWorkspace='EMU00020896')  # 20180607084524508312000
RenameWorkspace(InputWorkspace='35566', OutputWorkspace='Rename2')  # 20180607084534289381000
RenameWorkspace(InputWorkspace='EMU00020896', OutputWorkspace='Rename1')  # 20180607084539748304000
RenameWorkspace(InputWorkspace='EMU00011892', OutputWorkspace='Rename3')  # 20180607084544504881000
Fit(Function='name=DynamicKuboToyabe,BinWidth=0.050000000000000003,Asym=5.83382,Delta=5.63288,Field=447.873,Nu=8.53636e-09', InputWorkspace='Rename1', IgnoreInvalidData=True, Output='Rename1_fit', OutputCompositeMembers=True, ConvolveMembers=True)  # 20180607084707715350000
Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename2', IgnoreInvalidData=True, Output='Rename2_fit', OutputCompositeMembers=True, ConvolveMembers=True)  # 20180607084731287274000
Fit(Function='name=Abragam,A=-500.565,Omega=944.105,Phi=-2.97876,Sigma=230.906,Tau=5.54415e+06', InputWorkspace='Rename1_fit_Workspace', CreateOutput=True, Output='Rename1_fit_Workspace_1', CalcErrors=True)  # 20180607084836466082000
Fit(Function='name=Abragam,A=343210,Omega=-91853.1,Phi=-1.51509,Sigma=11920.5,Tau=2.80013e+13', InputWorkspace='Rename2_fit_Workspace', CreateOutput=True, Output='Rename2_fit_Workspace_1', CalcErrors=True)  # 20180607084836471228000
GroupWorkspaces(InputWorkspaces='Rename1_fit_Workspace_1_Workspace,Rename2_fit_Workspace_1_Workspace', OutputWorkspace='Rename3_fit_Workspaces')  # 20180607084836476434000
RenameWorkspace(InputWorkspace='Rename1_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential1')  # 20180607084906916756000
RenameWorkspace(InputWorkspace='Rename2_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential2')  # 20180607084914254310000
Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename3', IgnoreInvalidData=True, Output='Rename3_fit', OutputCompositeMembers=True, ConvolveMembers=True)  # 20180607085219946558000
Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename2_fit_Workspace', CreateOutput=True, Output='Rename2_fit_Workspace_1', CalcErrors=True)  # 20180607085248384165000
Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename3_fit_Workspace', CreateOutput=True, Output='Rename3_fit_Workspace_1', CalcErrors=True)  # 20180607085248390417000
GroupWorkspaces(InputWorkspaces='Rename2_fit_Workspace_1_Workspace,Rename3_fit_Workspace_1_Workspace', OutputWorkspace='Rename3_fit_Workspaces')  # 20180607085248434043000
RenameWorkspace(InputWorkspace='Rename3_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential3')  # 20180607085259889025000
RenameWorkspace(InputWorkspace='Rename2_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential4')  # 20180607085306565223000
Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename3_fit_Workspace', CreateOutput=True, Output='Rename3_fit_Workspace_1', CalcErrors=True)  # 20180607085340743253000
Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='Rename1_fit_Workspace', CreateOutput=True, Output='Rename1_fit_Workspace_1', CalcErrors=True)  # 20180607085340747109000
GroupWorkspaces(InputWorkspaces='Rename3_fit_Workspace_1_Workspace,Rename1_fit_Workspace_1_Workspace', OutputWorkspace='Rename3_fit_Workspaces')  # 20180607085340855026000
RenameWorkspace(InputWorkspace='Rename3_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential5')  # 20180607085355191542000
RenameWorkspace(InputWorkspace='Rename1_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential6')  # 20180607085402367864000

for i in range(1,7):
    ws = mtd['Sqquential%s' % i]
    write_algorithm_history(ws)

