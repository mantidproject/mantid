#pylint: disable=no-init,attribute-defined-outside-init, too-few-public-methods
import stresstesting
import os
from abc import ABCMeta, abstractmethod
from mantid.simpleapi import *
from IndirectImport import is_supported_f2py_platform

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

class QLresTest(stresstesting.MantidStressTest):

    def skipTests(self):
        if is_supported_f2py_platform():
            return False
        else:
            return True

    def runTest(self):
        import IndirectBayes as Main
        nbins = ['1', '1']
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        rsname = ''
        wfile = ''
        erange = [-0.5, 0.5]
        fitOp = [True, 'Sloping', False, False] #elastic, background, width, resnorm
        loopOp = False
        plotOp = False
        saveOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        Main.QLRun('QL',sname,rname,rsname,erange,nbins,fitOp,wfile,loopOp,plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-4
        return 'irs26176_graphite002_QLr_Workspace_0','ISISIndirectBayes_QlresTest.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QLr.lpt','irs26176_graphite002_QLr.ql1',
                     'irs26176_graphite002_QLr.ql2','irs26176_graphite002_QLr.ql3',
                     'irs26176_graphite002_QLr_Parameters.nxs']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================

class ResNormTest(stresstesting.MantidStressTest):

    def skipTests(self):
        if is_supported_f2py_platform():
            return False
        else:
            return True

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

class QuestTest(stresstesting.MantidStressTest):

    def skipTests(self):
        if is_supported_f2py_platform():
            return False
        else:
            return True

    def runTest(self):
        import IndirectBayes as Main
        nbins = [1, 1]
        nbs = [50, 30]
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        erange = [-0.5, 0.5]
        fitOp = [True, 'Sloping', False, False] #elastic, background, width, resnorm
        loopOp = False
        plotOp = 'None'
        saveOp = False

        spath = sname+'.nxs'   # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        Main.QuestRun(sname,rname,nbs,erange,nbins,fitOp,loopOp,plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-1
        return 'irs26176_graphite002_Qst_Fit','ISISIndirectBayes_QuestTest.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_Qst.lpt','irs26176_graphite002_Qss.ql2',
                     'irs26176_graphite002_Qsb.ql1']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================

class QSeTest(stresstesting.MantidStressTest):

    def skipTests(self):
        if is_supported_f2py_platform():
            return False
        else:
            return True

    def runTest(self):
        import IndirectBayes as Main
        nbins = ['1', '1']
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        rsname = ''
        wfile = ''
        erange = [-0.5, 0.5]
        fitOp = [True, 'Sloping', False, False] #elastic, background, width, resnorm
        loopOp = False
        plotOp = False
        saveOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        Main.QLRun('QSe',sname,rname,rsname,erange,nbins,fitOp,wfile,loopOp,plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-1
        return 'irs26176_graphite002_QSe_Workspace_0','ISISIndirectBayes_QSeTest.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QSe_Parameters.nxs', 'irs26176_graphite002_Qse.qse',
                     'irs26176_graphite002_Qse.lpt']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================

class QLDataTest(stresstesting.MantidStressTest):

    def skipTests(self):
        if is_supported_f2py_platform():
            return False
        else:
            return True

    def runTest(self):
        import IndirectBayes as Main
        nbins = ['1', '1']
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_red'
        rsname = ''
        wfile = ''
        erange = [-0.5, 0.5]
        fitOp = [True, 'Sloping', False, False] #elastic, background, width, resnorm
        loopOp = False
        plotOp = False
        saveOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        Main.QLRun('QL',sname,rname,rsname,erange,nbins,fitOp,wfile,loopOp,plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-4
        return 'irs26176_graphite002_QLd_Workspace_0','ISISIndirectBayes_QLDataTest.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QLd.lpt','irs26176_graphite002_QLd.ql1',
                     'irs26176_graphite002_QLd.ql2','irs26176_graphite002_QLd.ql3',
                     'irs26176_graphite002_QLd_Parameters.nxs']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================

class QLResNormTest(stresstesting.MantidStressTest):

    def skipTests(self):
        if is_supported_f2py_platform():
            return False
        else:
            return True

    def runTest(self):
        import IndirectBayes as Main

        nbins = ['1', '1']
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        rsname = 'irs26173_graphite002_ResNorm'
        wfile = ''
        erange = [-0.5, 0.5]
        fitOp = [True, 'Sloping', False, True] #elastic, background, width, resnorm
        loopOp = True
        plotOp = False
        saveOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        rspath = rsname+'_Paras.nxs'    # path name for resNorm nxs file
        LoadNexusProcessed(Filename=rspath, OutputWorkspace=rsname)
        Main.QLRun('QL',sname,rname,rsname,erange,nbins,fitOp,wfile,loopOp,plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-1
        return 'irs26176_graphite002_QLr_Workspaces','ISISIndirectBayes_QLr_ResNorm_Test.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QLd.lpt','irs26176_graphite002_QLd.ql1',
                     'irs26176_graphite002_QLd.ql2','irs26176_graphite002_QLd.ql3',
                     'irs26176_graphite002_QLd_Parameters.nxs']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================

class QLWidthTest(stresstesting.MantidStressTest):

    def skipTests(self):
        if is_supported_f2py_platform():
            return False
        else:
            return True

    def runTest(self):
        import IndirectBayes as Main

        nbins = ['1', '1']
        sname = 'irs26176_graphite002_red'
        rname = 'irs26173_graphite002_res'
        rsname = ''
        wfile = 'irs26176_graphite002_width_water.dat'
        erange = [-0.5, 0.5]
        fitOp = [True, 'Sloping', True, False] #elastic, background, width, resnorm
        loopOp = False
        plotOp = False
        saveOp = False

        spath = sname+'.nxs'    # path name for sample nxs file
        LoadNexusProcessed(Filename=spath, OutputWorkspace=sname)
        rpath = rname+'.nxs'    # path name for res nxs file
        LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
        Main.QLRun('QL',sname,rname,rsname,erange,nbins,fitOp,wfile,loopOp,plotOp,saveOp)

    def validate(self):
        self.tolerance = 1e-1
        return 'irs26176_graphite002_QLr_Workspace_0','ISISIndirectBayes_QLr_width_Test.nxs'

    def cleanup(self):
        filenames = ['irs26176_graphite002_QLd.lpt','irs26176_graphite002_QLd.ql1',
                     'irs26176_graphite002_QLd.ql2','irs26176_graphite002_QLd.ql3',
                     'irs26176_graphite002_QLd_Parameters.nxs']
        _cleanup_files(config['defaultsave.directory'], filenames)

#==============================================================================

class JumpFitFunctionTestBase(stresstesting.MantidStressTest):

    __metaclass__ = ABCMeta

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)

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

        self._function = 'name=ChudleyElliot,Tau=3.31,L=1.42'
        self.tolerance = 5e-3

    def get_reference_files(self):
        return 'ISISIndirectBayes_JumpCETest.nxs'

#==============================================================================

class JumpHallRossTest(JumpFitFunctionTestBase):

    def __init__(self):
        JumpFitFunctionTestBase.__init__(self)

        self._function = 'name=HallRoss,Tau=2.7,L=1.75'
        self.tolerance = 1e-5

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

        self._function = 'name=TeixeiraWater,Tau=1.6,L=0.4'
        self.tolerance = 1e-3

    def get_reference_files(self):
        return 'ISISIndirectBayes_JumpTeixeiraTest.nxs'

#==============================================================================
