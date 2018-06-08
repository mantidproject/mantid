from __future__ import print_function
import sys
sys.path.append('/Applications/MantidPlot_312.app/Contents/MacOS')
from mantid.simpleapi import *

data_directory='/Users/keithbutler/Documents/Mantid/Data/Muons/Diffusion/EMU000'
workspace_list=[]

for i in range(11888, 11899):
    Load(Filename='%s%s.nxs' % (data_directory, i), OutputWorkspace='%s' % i)
    workspace_list.append(i)

for i in range(18849, 18864):
    Load(Filename='%s%s.nxs' % (data_directory, i), OutputWorkspace='%s' % i)
    workspace_list.append(i)

for i in range(19625, 19641):
    Load(Filename='%s%s.nxs' % (data_directory, i), OutputWorkspace='%s' % i)
    workspace_list.append(i)

for i in workspace_list:
    Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='%s' % i, IgnoreInvalidData=True, Output='%s_fit' %i, OutputCompositeMembers=True, ConvolveMembers=True)
    RenameWorkspace(InputWorkspace='%s_fit_Workspace' % i, OutputWorkspace='%s_fit' % i)

for i in workspace_list:
    Rebin(InputWorkspace='%s' % i, OutputWorkspace='%s_rebin' % i, Params='0.5')
    Rebin(InputWorkspace='%s_fit' % i, OutputWorkspace='%s_fit_rebin' % i, Params='0.5')

for i in workspace_list:
    Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='%s_fit' % i, IgnoreInvalidData=True, Output='%s_fit_fit' %i, OutputCompositeMembers=True, ConvolveMembers=True)
    RenameWorkspace(InputWorkspace='%s_fit_fit_Workspace' % i, OutputWorkspace='%s_fit' % i)
    Fit(Function='name=ExpDecayMuon,A=4306.05,Lambda=0.458289', InputWorkspace='%s_fit_rebin' % i, IgnoreInvalidData=True, Output='%s_fit_rebin_fit' %i, OutputCompositeMembers=True, ConvolveMembers=True)
    RenameWorkspace(InputWorkspace='%s_fit_rebin_fit_Workspace' % i, OutputWorkspace='%s_fit_rebin' % i)

for i in workspace_list:
    ws = mtd['%s_fit_rebin' % i]
    write_algorithm_history(ws)
    ws = mtd['%s_fit' % i]
    write_algorithm_history(ws)

