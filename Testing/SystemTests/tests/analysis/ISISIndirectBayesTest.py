# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,attribute-defined-outside-init, too-few-public-methods
from __future__ import (absolute_import, division, print_function)
import systemtesting
import os
from abc import ABCMeta, abstractmethod
from mantid.simpleapi import *
import platform
from six import with_metaclass

#==============================================================================


def _cleanup_files(dirname, filenames):
    """
       Attempts to remove each filename from
       the given directory
    """
    for filename in filenames:
        path = os.path.join(dirname, filename)
        try:
            os.remove(path)
        except OSError:
            pass

#==============================================================================


class QLresTest(systemtesting.MantidSystemTest):

    def skipTests(self):
        return not platform.system() == "Windows"

    def runTest(self):
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        e_min = -0.5
        e_max = 0.5
        background = 'Sloping'
        fixed_width = False
        loopOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        BayesQuasi(Program = 'QL',
                   SampleWorkspace=sname,
                   ResolutionWorkspace=rname,
                   MinRange=e_min,
                   MaxRange=e_max,
                   Background = background,
                   FixedWidth = fixed_width,
                   Loop = loopOp,
                   OutputWorkspaceFit='irs26176_graphite002_QLr_Workspace',
                   OutputWorkspaceResult='irs26176_graphite002_QLr_Result',
                   OutputWorkspaceProb='irs26176_graphite002_QLr_Probability')

    def validate(self):
        self.tolerance = 1e-4
        self.disableChecking.append('SpectraMap')
        return 'irs26176_graphite002_QLr_Workspace_0','ISISIndirectBayes_QlresTest.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QLr.lpt','irs26176_graphite002_QLr.ql1',
                     'irs26176_graphite002_QLr.ql2','irs26176_graphite002_QLr.ql3',
                     'irs26176_graphite002_QLr_Parameters.nxs']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================


class ResNormTest(systemtesting.MantidSystemTest):

    def skipTests(self):
        return not platform.system() == "Windows"

    def runTest(self):
        import IndirectBayes as Main
        nbin = '1'
        vname = 'irs26173_graphite002_red'
        rname = 'irs26173_graphite002_res'
        erange = [-0.2, 0.2]
        plotOp = False
        saveOp = False

        vpath = vname+'.nxs'    # path name for van nxs file
        LoadNexusProcessed(Filename=vpath, OutputWorkspace=vname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        Main.ResNormRun(vname,rname,erange,nbin,plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-4
        self.disableChecking.append("SpectraMap")
        return 'irs26173_graphite002_ResNorm_Fit','ISISIndirectBayes_ResNormTest.nxs'

    def cleanup(self):
        filenames = ['irs26173_graphite002_resnrm.lpt']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================


class QuestTest(systemtesting.MantidSystemTest):

    def skipTests(self):
        return not platform.system() == "Windows"

    def runTest(self):
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'

        spath = sname+'.nxs'   # path name for sample nxs file
        sample = LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        res = LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        BayesStretch(SampleWorkspace=sample,
                     ResolutionWorkspace=res,
                     EMin=-0.5,
                     EMax=0.5,
                     SampleBins=1,
                     Elastic=True,
                     Background='Sloping',
                     NumberSigma=30,
                     NumberBeta=50,
                     Loop=False,
                     OutputWorkspaceFit='fit_group',
                     OutputWorkspaceContour='contour_group')

    def validate(self):
        self.tolerance = 1e-1
        self.disableChecking.append('SpectraMap')
        return 'irs26176_graphite002_Stretch_Fit','ISISIndirectBayes_QuestTest.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_Qst.lpt','irs26176_graphite002_Qss.ql2',
                     'irs26176_graphite002_Qsb.ql1']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================


class QSeTest(systemtesting.MantidSystemTest):

    def skipTests(self):
        return not platform.system() == "Windows"

    def runTest(self):
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        e_min = -0.5
        e_max = 0.5
        background = 'Sloping'
        fixed_width = False
        loopOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        BayesQuasi(Program = 'QSe',
                   SampleWorkspace=sname,
                   ResolutionWorkspace=rname,
                   MinRange=e_min,
                   MaxRange=e_max,
                   Background = background,
                   FixedWidth = fixed_width,
                   Loop = loopOp,
                   OutputWorkspaceFit='irs26176_graphite002_QSe_Workspace',
                   OutputWorkspaceResult='irs26176_graphite002_QSe_Result')

    def validate(self):
        self.tolerance = 1e-1
        self.disableChecking.append('SpectraMap')
        return 'irs26176_graphite002_QSe_Workspace_0','ISISIndirectBayes_QSeTest.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QSe_Parameters.nxs', 'irs26176_graphite002_Qse.qse',
                     'irs26176_graphite002_Qse.lpt']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================


class QLDataTest(systemtesting.MantidSystemTest):

    def skipTests(self):
        return not platform.system() == "Windows"

    def runTest(self):
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_red'
        e_min = -0.5
        e_max = 0.5
        background = 'Sloping'
        fixed_width = False
        loopOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        BayesQuasi(Program = 'QL',
                   SampleWorkspace=sname,
                   ResolutionWorkspace=rname,
                   MinRange=e_min,
                   MaxRange=e_max,
                   Background = background,
                   FixedWidth = fixed_width,
                   Loop = loopOp,
                   OutputWorkspaceFit='irs26176_graphite002_QLr_Workspace',
                   OutputWorkspaceResult='irs26176_graphite002_QLr_Result',
                   OutputWorkspaceProb='irs26176_graphite002_QLr_Probability')

    def validate(self):
        self.tolerance = 1e-4
        self.disableChecking.append('SpectraMap')
        return 'irs26176_graphite002_QLd_Workspace_0','ISISIndirectBayes_QLDataTest.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QLd.lpt','irs26176_graphite002_QLd.ql1',
                     'irs26176_graphite002_QLd.ql2','irs26176_graphite002_QLd.ql3',
                     'irs26176_graphite002_QLd_Parameters.nxs']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================


class QLResNormTest(systemtesting.MantidSystemTest):

    def skipTests(self):
        return not platform.system() == "Windows"

    def runTest(self):
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        rsname = 'irs26173_graphite002_ResNorm'
        e_min = -0.5
        e_max = 0.5
        background = 'Sloping'
        fixed_width = False
        use_resNorm = True

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        rspath = rsname+'_Paras.nxs'    # path name for resNorm nxs file
        LoadNexusProcessed(Filename=rspath, OutputWorkspace=rsname)
        BayesQuasi(Program = 'QL',
                   SampleWorkspace=sname,
                   ResolutionWorkspace=rname,
                   ResNormWorkspace=rsname,
                   MinRange=e_min,
                   MaxRange=e_max,
                   Background = background,
                   FixedWidth = fixed_width,
                   UseResNorm = use_resNorm,
                   OutputWorkspaceFit='irs26176_graphite002_QLr_Workspace',
                   OutputWorkspaceResult='irs26176_graphite002_QLr_Result',
                   OutputWorkspaceProb='irs26176_graphite002_QLr_Probability')

    def validate(self):
        self.tolerance = 1e-1
        self.disableChecking.append('SpectraMap')
        return 'irs26176_graphite002_QLr_Workspaces','ISISIndirectBayes_QLr_ResNorm_Test.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QLd.lpt','irs26176_graphite002_QLd.ql1',
                     'irs26176_graphite002_QLd.ql2','irs26176_graphite002_QLd.ql3',
                     'irs26176_graphite002_QLd_Parameters.nxs']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================


class QLWidthTest(systemtesting.MantidSystemTest):

    def skipTests(self):
        return not platform.system() == "Windows"

    def runTest(self):
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        wfile = 'irs26176_graphite002_width_water.dat'
        e_min = -0.5
        e_max = 0.5
        background = 'Sloping'
        loopOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        BayesQuasi(Program = 'QL',
                   SampleWorkspace=sname,
                   ResolutionWorkspace=rname,
                   MinRange=e_min,
                   MaxRange=e_max,
                   Background = background,
                   WidthFile = wfile,
                   Loop = loopOp,
                   OutputWorkspaceFit='irs26176_graphite002_QLr_Workspace',
                   OutputWorkspaceResult='irs26176_graphite002_QLr_Result',
                   OutputWorkspaceProb='irs26176_graphite002_QLr_Probability')

    def validate(self):
        self.tolerance = 1e-1
        self.disableChecking.append('SpectraMap')
        return 'irs26176_graphite002_QLr_Workspace_0','ISISIndirectBayes_QLr_width_Test.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QLd.lpt','irs26176_graphite002_QLd.ql1',
                     'irs26176_graphite002_QLd.ql2','irs26176_graphite002_QLd.ql3',
                     'irs26176_graphite002_QLd_Parameters.nxs']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================


class JumpFitFunctionTestBase(with_metaclass(ABCMeta, systemtesting.MantidSystemTest)):

    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)

        self._sample_name = 'irs26176_graphite002_QLr_Workspace'
        self._q_range = [0.6, 1.705600]
        self._width_index = 2

        self._function = ''

    @abstractmethod
    def get_reference_files(self):
        '''Returns the name of the reference files to compare against.'''
        raise NotImplementedError("Implmenent get_reference_files to return "
                                  "the names of the files to compare against.")

    def runTest(self):
        # Load file
        filename = self._sample_name + '.nxs'
        LoadNexusProcessed(Filename=filename,
                           OutputWorkspace=self._sample_name)

        # Extract the width spectrum
        ExtractSingleSpectrum(InputWorkspace=self._sample_name,
                              OutputWorkspace=self._sample_name,
                              WorkspaceIndex=self._width_index)

        # Data must be in HWHM
        Scale(InputWorkspace=self._sample_name,
              OutputWorkspace=self._sample_name,
              Factor=0.5)

        Fit(InputWorkspace=self._sample_name,
            Function=self._function,
            StartX=self._q_range[0],
            EndX=self._q_range[1],
            CreateOutput=True,
            Output=self._sample_name)

    def validate(self):
        return self._sample_name + '_Workspace', self.get_reference_files()

#==============================================================================


class JumpCETest(JumpFitFunctionTestBase):

    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = 'name=ChudleyElliot,Tau=1.42,L=2.42'
        self.tolerance = 5e-3

    def get_reference_files(self):
        return 'ISISIndirectBayes_JumpCETest.nxs'

#==============================================================================


class JumpHallRossTest(JumpFitFunctionTestBase):

    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = 'name=HallRoss,Tau=2.7,L=1.75'
        self.tolerance = 5e-3

    def get_reference_files(self):
        return 'ISISIndirectBayes_JumpHallRossTest.nxs'

#==============================================================================


class JumpFickTest(JumpFitFunctionTestBase):

    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = 'name=FickDiffusion,D=0.07'
        self.tolerance = 5e-4

    def get_reference_files(self):
        return 'ISISIndirectBayes_JumpFickTest.nxs'

#==============================================================================


class JumpTeixeiraTest(JumpFitFunctionTestBase):

    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = 'name=TeixeiraWater,Tau=1.6,L=0.9'
        self.tolerance = 1e-3

    def get_reference_files(self):
        return 'ISISIndirectBayes_JumpTeixeiraTest.nxs'

#==============================================================================
