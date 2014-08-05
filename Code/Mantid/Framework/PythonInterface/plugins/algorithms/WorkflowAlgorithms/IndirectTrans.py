from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
from mantid import config, logger
from IndirectImport import import_mantidplot
import os.path, math, numpy as np

class IndirectTrans(PythonAlgorithm):
 
    def category(self):
        return "Workflow;PthonAlgorithms;Indirect"

    def PyInit(self):
        self.declareProperty(name='SampleFile', defaultValue='', doc='Sample file')
        self.declareProperty(name='CanFile', defaultValue='', doc='Can file')
        self.declareProperty(name='Verbose', defaultValue=False, doc='Enable more verbose logging message')
        self.declareProperty(name='Plot', defaultValue=False, doc='Plot result workspace')
        self.declareProperty(name='Save', defaultValue=False, doc='Save result workspace to nexus file')

    def PyExec(self):
        from IndirectCommon import StartTime, EndTime
        StartTime('IndirectTrans')

        sample_filepath = self.getProperty("SampleFile").value
        can_filepath = self.getProperty("CanFile").value
        verbose = self.getProperty("Verbose").value
        plot = self.getProperty("Plot").value
        save = self.getProperty("Save").value

        sample_filename = os.path.basename(sample_filepath)
        ws_basename, ext = os.path.splitext(sample_filename)

        self.TransMon(ws_basename, 'Sam', sample_filepath, verbose)
        self.TransMon(ws_basename, 'Can', can_filepath, verbose)

        # Divide sample and can workspaces
        samWS = ws_basename + '_Sam'
        canWS = ws_basename + '_Can'
        trWS = ws_basename + '_Trans'
        Divide(LHSWorkspace=samWS, RHSWorkspace=canWS, OutputWorkspace=trWS)
        trans = np.average(mtd[trWS].readY(0))

        # Group workspaces
        transWS = ws_basename + '_Transmission'
        group = samWS + ',' + canWS + ',' + trWS
        GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=transWS)

        if verbose:
            logger.notice('Transmission : ' + str(trans))

        # Save the tranmissin workspace group to a nexus file
        if save:
            workdir = config['defaultsave.directory']
            path = os.path.join(workdir, transWS + '.nxs')
            SaveNexusProcessed(InputWorkspace=transWS, Filename=path)
            if verbose:
                logger.notice('Output file created : ' + path)

        # Plot spectra from transmission workspace
        if plot:
            mp = import_mantidplot()
            tr_plot = mp.plotSpectrum(transWS, 0)

        EndTime('IndirectTrans')

    def UnwrapMon(self, inWS):
        # Unwrap monitor - inWS contains M1,M2,S1 - outWS contains unwrapped Mon
        # Unwrap s1>2 to L of S2 (M2) ie 38.76  Ouput is in wavelength
        out, join = UnwrapMonitor(InputWorkspace=inWS, LRef='37.86')
        outWS = 'out'

        # Fill bad (dip) in spectrum
        RemoveBins(InputWorkspace=outWS, OutputWorkspace=outWS, Xmin=join-0.001, Xmax=join+0.001, Interpolation="Linear")
        FFTSmooth(InputWorkspace=outWS, OutputWorkspace=outWS, WorkspaceIndex=0, IgnoreXBins=True) # Smooth - FFT

        DeleteWorkspace(inWS) # delete monWS

        return outWS

    def TransMon(self, ws_basename, type, file, verbose):
        if verbose:
            logger.notice('Raw file : ' + file)

        LoadRaw(Filename=file, OutputWorkspace='__m1', SpectrumMin=1, SpectrumMax=1)
        LoadRaw(Filename=file, OutputWorkspace='__m2', SpectrumMin=2, SpectrumMax=2)		
        LoadRaw(Filename=file, OutputWorkspace='__det', SpectrumMin=3, SpectrumMax=3)	

        # Check for single or multiple time regimes
        MonTCBstart = mtd['__m1'].readX(0)[0]
        SpecTCBstart = mtd['__det'].readX(0)[0]	

        DeleteWorkspace('__det') # delete monWS
        monWS = '__Mon'

        if (SpecTCBstart == MonTCBstart):
            monWS = self.UnwrapMon('__m1') # unwrap the monitor spectrum and convert to wavelength
            RenameWorkspace(InputWorkspace=monWS, OutputWorkspace='__Mon1')		
        else:
            ConvertUnits(InputWorkspace='__m1', OutputWorkspace='__Mon1', Target="Wavelength")
            ConvertUnits(InputWorkspace='__m2', OutputWorkspace='__Mon2', Target="Wavelength")

            DeleteWorkspace('__m2') # delete monWS

            Xin = mtd['__Mon1'].readX(0)
            xmin1 = mtd['__Mon1'].readX(0)[0]
            xmax1 = mtd['__Mon1'].readX(0)[len(Xin)-1]
            Xin = mtd['__Mon2'].readX(0)
            xmin2 = mtd['__Mon2'].readX(0)[0]
            xmax2 = mtd['__Mon2'].readX(0)[len(Xin)-1]
            wmin = max(xmin1, xmin2)
            wmax = min(xmax1, xmax2)

            CropWorkspace(InputWorkspace='__Mon1', OutputWorkspace='__Mon1', XMin=wmin, XMax=wmax)
            RebinToWorkspace(WorkspaceToRebin='__Mon2', WorkspaceToMatch='__Mon1', OutputWorkspace='__Mon2')
            monWS = ws_basename +'_' + type
            Divide(LHSWorkspace='__Mon2', RHSWorkspace='__Mon1', OutputWorkspace=monWS)

            DeleteWorkspace('__Mon1') # delete monWS
            DeleteWorkspace('__Mon2') # delete monWS

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectTrans)
