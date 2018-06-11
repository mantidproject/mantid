from matid.simpleapi import *

Load(Filename='/Users/keithbutler/Documents/Mantid/Data/Muons/Diffusion/EMU00011892.nxs', OutputWorkspace='EMU00011892',
     LoaderName='LoadMuonNexus', LoaderVersion='1', SpectrumMin='2147483647',
     SpectrumMax='2147483647', SpectrumList='', AutoGroup='0', EntryNumber='0', MainFieldDirection='Transverse',
     TimeZero='0', FirstGoodData='0', DeadTimeTable='', DetectorGroupingTable='')  # 20180607125147352135000
Load(Filename='/Users/keithbutler/Documents/Mantid/Data/Muons/Nickel/35566.NXS', OutputWorkspace='35566',
     LoaderName='LoadMuonNexus', LoaderVersion='1', SpectrumMin='2147483647', SpectrumMax='2147483647',
     SpectrumList='', AutoGroup='0', EntryNumber='0', MainFieldDirection='Transverse', TimeZero='0', FirstGoodData='0',
     DeadTimeTable='', DetectorGroupingTable='')  # 20180607125147404698000
Load(Filename='/Users/keithbutler/Documents/Mantid/Data/Muons/Cu/Cu_Training_Course_Data/EMU00020896.nxs',
     OutputWorkspace='EMU00020896', LoaderName='LoadMuonNexus', LoaderVersion='1', SpectrumMin='2147483647',
     SpectrumMax='2147483647', SpectrumList='', AutoGroup='0', EntryNumber='0', MainFieldDirection='Transverse',
     TimeZero='0', FirstGoodData='0', DeadTimeTable='', DetectorGroupingTable='')  # 20180607125147436609000
RenameWorkspace(InputWorkspace='35566', OutputWorkspace='Rename2',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147479705000
RenameWorkspace(InputWorkspace='EMU00020896', OutputWorkspace='Rename1',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147481282000
RenameWorkspace(InputWorkspace='EMU00011892', OutputWorkspace='Rename3',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147482781000
Fit(Function='name=DynamicKuboToyabe,BinWidth=0.050000000000000003,Asym=98.3009,Delta=182.914,Field=12157.8,Nu=2.97055',
    InputWorkspace='Rename1', IgnoreInvalidData='1', DomainType='Simple', EvaluationType='CentrePoint', PeakRadius='0',
    Ties='', Constraints='', MaxIterations='500', OutputStatus='success', OutputChi2overDoF='102.41756677466226',
    Minimizer='Levenberg-Marquardt', CostFunction='Least squares', CreateOutput='0',
    Output='Rename1_fit', CalcErrors='0', OutputCompositeMembers='1', ConvolveMembers='1', OutputParametersOnly='0',
    WorkspaceIndex='0', StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307', Normalise='0', Exclude='',
    OutputNormalisedCovarianceMatrix='Rename1_fit_NormalisedCovarianceMatrix',
    OutputParameters='Rename1_fit_Parameters', OutputWorkspace='Rename1_fit_Workspace')  # 20180607125147485569000
Fit(Function='name=ExpDecayMuon,A=194.117,Lambda=0.158028', InputWorkspace='Rename2', IgnoreInvalidData='1',
    DomainType='Simple', EvaluationType='CentrePoint', PeakRadius='0', Ties='', Constraints='', MaxIterations='500',
    OutputStatus='success', OutputChi2overDoF='519.14701777922426', Minimizer='Levenberg-Marquardt',
    CostFunction='Least squares', CreateOutput='0', Output='Rename2_fit',
    CalcErrors='0', OutputCompositeMembers='1', ConvolveMembers='1', OutputParametersOnly='0', WorkspaceIndex='0',
    StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307', Normalise='0', Exclude='',
    OutputNormalisedCovarianceMatrix='Rename2_fit_NormalisedCovarianceMatrix',
    OutputParameters='Rename2_fit_Parameters', OutputWorkspace='Rename2_fit_Workspace')  # 20180607125147652203000
Fit(Function='name=Abragam,A=-647.97,Omega=688.603,Phi=2.22673,Sigma=198.394,Tau=-3.50029e+10',
    InputWorkspace='Rename1_fit_Workspace', IgnoreInvalidData='0', DomainType='Simple', EvaluationType='CentrePoint',
    PeakRadius='0', Ties='', Constraints='', MaxIterations='500',
    OutputStatus='Failed to converge after 500 iterations.', OutputChi2overDoF='117.92671157102517',
    Minimizer='Levenberg-Marquardt', CostFunction='Least squares', CreateOutput='1',
    Output='Rename1_fit_Workspace_1', CalcErrors='1', OutputCompositeMembers='0', ConvolveMembers='0',
    OutputParametersOnly='0', WorkspaceIndex='0', StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307',
    Normalise='0', Exclude='', OutputNormalisedCovarianceMatrix='Rename1_fit_Workspace_1_NormalisedCovarianceMatrix',
    OutputParameters='Rename1_fit_Workspace_1_Parameters',
    OutputWorkspace='Rename1_fit_Workspace_1_Workspace')  # 20180607125147656077000
Fit(Function='name=Abragam,A=343210,Omega=-91853.1,Phi=-1.55662,Sigma=11920.5,Tau=2.80013e+13',
    InputWorkspace='Rename2_fit_Workspace', IgnoreInvalidData='0', DomainType='Simple', EvaluationType='CentrePoint',
    PeakRadius='0', Ties='', Constraints='', MaxIterations='500', OutputStatus='success',
    OutputChi2overDoF='595.79597989949571', Minimizer='Levenberg-Marquardt', CostFunction='Least squares',
    CreateOutput='1', Output='Rename2_fit_Workspace_1',
    CalcErrors='1', OutputCompositeMembers='0', ConvolveMembers='0', OutputParametersOnly='0', WorkspaceIndex='0',
    StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307', Normalise='0', Exclude='',
    OutputNormalisedCovarianceMatrix='Rename2_fit_Workspace_1_NormalisedCovarianceMatrix',
    OutputParameters='Rename2_fit_Workspace_1_Parameters',
    OutputWorkspace='Rename2_fit_Workspace_1_Workspace')  # 20180607125147907534000
GroupWorkspaces(InputWorkspaces='Rename1_fit_Workspace_1_Workspace,Rename2_fit_Workspace_1_Workspace',
                OutputWorkspace='Rename3_fit_Workspaces')  # 20180607125147910648000
RenameWorkspace(InputWorkspace='Rename1_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential1',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147911831000
RenameWorkspace(InputWorkspace='Rename2_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential2',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147912974000
Fit(Function='name=ExpDecayMuon,A=52.5974,Lambda=0.144777', InputWorkspace='Rename3', IgnoreInvalidData='1',
    DomainType='Simple', EvaluationType='CentrePoint', PeakRadius='0', Ties='', Constraints='', MaxIterations='500',
    OutputStatus='success', OutputChi2overDoF='120.66222181102351', Minimizer='Levenberg-Marquardt',
    CostFunction='Least squares', CreateOutput='0', Output='Rename3_fit',
    CalcErrors='0', OutputCompositeMembers='1', ConvolveMembers='1', OutputParametersOnly='0', WorkspaceIndex='0',
    StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307', Normalise='0', Exclude='',
    OutputNormalisedCovarianceMatrix='Rename3_fit_NormalisedCovarianceMatrix',
    OutputParameters='Rename3_fit_Parameters', OutputWorkspace='Rename3_fit_Workspace')  # 20180607125147914843000
Fit(Function='name=ExpDecayMuon,A=54.4887,Lambda=0.0813372', InputWorkspace='Rename2_fit_Workspace',
    IgnoreInvalidData='0', DomainType='Simple', EvaluationType='CentrePoint', PeakRadius='0', Ties='', Constraints='',
    MaxIterations='500', OutputStatus='success', OutputChi2overDoF='564.90847025719688',
    Minimizer='Levenberg-Marquardt', CostFunction='Least squares', CreateOutput='1', Output='Rename2_fit_Workspace_1',
    CalcErrors='1', OutputCompositeMembers='0', ConvolveMembers='0', OutputParametersOnly='0', WorkspaceIndex='0',
    StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307', Normalise='0', Exclude='',
    OutputNormalisedCovarianceMatrix='Rename2_fit_Workspace_1_NormalisedCovarianceMatrix',
    OutputParameters='Rename2_fit_Workspace_1_Parameters',
    OutputWorkspace='Rename2_fit_Workspace_1_Workspace')  # 20180607125147919379000
Fit(Function='name=ExpDecayMuon,A=33.3401,Lambda=0.149432', InputWorkspace='Rename3_fit_Workspace',
    IgnoreInvalidData='0', DomainType='Simple', EvaluationType='CentrePoint', PeakRadius='0', Ties='', Constraints='',
    MaxIterations='500', OutputStatus='success', OutputChi2overDoF='124.76079185558328',
    Minimizer='Levenberg-Marquardt', CostFunction='Least squares', CreateOutput='1', Output='Rename3_fit_Workspace_1',
    CalcErrors='1', OutputCompositeMembers='0', ConvolveMembers='0', OutputParametersOnly='0', WorkspaceIndex='0',
    StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307', Normalise='0', Exclude='',
    OutputNormalisedCovarianceMatrix='Rename3_fit_Workspace_1_NormalisedCovarianceMatrix',
    OutputParameters='Rename3_fit_Workspace_1_Parameters',
    OutputWorkspace='Rename3_fit_Workspace_1_Workspace')  # 20180607125147922512000
GroupWorkspaces(InputWorkspaces='Rename2_fit_Workspace_1_Workspace,Rename3_fit_Workspace_1_Workspace',
                OutputWorkspace='Rename3_fit_Workspaces')  # 20180607125147928442000
RenameWorkspace(InputWorkspace='Rename3_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential3',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147929841000
RenameWorkspace(InputWorkspace='Rename2_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential4',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147930988000
Fit(Function='name=ExpDecayMuon,A=33.3401,Lambda=0.149432', InputWorkspace='Rename3_fit_Workspace',
    IgnoreInvalidData='0', DomainType='Simple', EvaluationType='CentrePoint', PeakRadius='0', Ties='', Constraints='',
    MaxIterations='500', OutputStatus='success', OutputChi2overDoF='124.76079185558328',
    Minimizer='Levenberg-Marquardt', CostFunction='Least squares', CreateOutput='1', Output='Rename3_fit_Workspace_1',
    CalcErrors='1', OutputCompositeMembers='0', ConvolveMembers='0', OutputParametersOnly='0', WorkspaceIndex='0',
    StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307', Normalise='0', Exclude='',
    OutputNormalisedCovarianceMatrix='Rename3_fit_Workspace_1_NormalisedCovarianceMatrix',
    OutputParameters='Rename3_fit_Workspace_1_Parameters',
    OutputWorkspace='Rename3_fit_Workspace_1_Workspace')  # 20180607125147932778000
Fit(Function='name=ExpDecayMuon,A=29.0793,Lambda=0.146765', InputWorkspace='Rename1_fit_Workspace',
    IgnoreInvalidData='0', DomainType='Simple', EvaluationType='CentrePoint', PeakRadius='0', Ties='', Constraints='',
    MaxIterations='500', OutputStatus='success', OutputChi2overDoF='112.21271103112565',
    Minimizer='Levenberg-Marquardt', CostFunction='Least squares', CreateOutput='1', Output='Rename1_fit_Workspace_1',
    CalcErrors='1', OutputCompositeMembers='0', ConvolveMembers='0', OutputParametersOnly='0', WorkspaceIndex='0',
    StartX='8.9884656743115785e+307', EndX='8.9884656743115785e+307', Normalise='0', Exclude='',
    OutputNormalisedCovarianceMatrix='Rename1_fit_Workspace_1_NormalisedCovarianceMatrix',
    OutputParameters='Rename1_fit_Workspace_1_Parameters',
    OutputWorkspace='Rename1_fit_Workspace_1_Workspace')  # 20180607125147938613000
GroupWorkspaces(InputWorkspaces='Rename3_fit_Workspace_1_Workspace,Rename1_fit_Workspace_1_Workspace',
                OutputWorkspace='Rename3_fit_Workspaces')  # 20180607125147943998000
RenameWorkspace(InputWorkspace='Rename3_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential5',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147945195000
RenameWorkspace(InputWorkspace='Rename1_fit_Workspace_1_Workspace', OutputWorkspace='Sqquential6',
                RenameMonitors='0', OverwriteExisting='1')  # 20180607125147946346000
