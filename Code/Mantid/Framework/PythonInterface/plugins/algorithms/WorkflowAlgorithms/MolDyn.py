from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *
import os

import numpy as np

from IndirectCommon import ExtractFloat, ExtractInt
from IndirectNeutron import ChangeAngles, InstrParas, RunParas


def SplitLine(a):
    elements = a.split()  # split line on character
    extracted = []
    for n in elements:
        extracted.append(float(n))
    return extracted  # values as list

def FindDimensions(a, Verbose):
    ldim = FindStarts(a, 'dimensions', 0)
    lQ = FindTabStarts(a, 'NQVALUES', 0)
    lT = FindTabStarts(a, 'NTIMES', 0)
    lF = FindTabStarts(a, 'NFREQUENCIES', 0)
    Qel = a[lQ].split()
    nQ = int(Qel[2])
    Tel = a[lT].split()
    nT = int(Tel[2])
    Fel = a[lF].split()
    nF = int(Tel[2])
    if Verbose:
        logger.notice(a[2][1:-1])
        logger.notice(a[3][1:-1])
        logger.notice(a[6][1:-1])
    return nQ, nT, nF

def FindStarts(asc, c, l1):
    for l in range(l1, len(asc)):
        char = asc[l]
        if char.startswith(c):
            line = l
            break
    return line

def FindTabStarts(asc, c, l1):
    for l in range(l1, len(asc)):
        char = asc[l][1:]
        if char.startswith(c):
            line = l
            break
    return line

def FindEnds(asc, c, l1):
    for l in range(l1, len(asc)):
        char = asc[l]
        if char.endswith(c):
            line = l
            break
    return line

def FindChar(asc, c, l1):
    for l in range(l1, len(asc)):
        char = asc[l]
        if char.find(c):
            line = l
            break
    return line

def MakeList(a, l1, l2):
    asc = ''
    for m in range(l1, l2 + 1):
        asc += a[m]
        alist = asc.split(',')
    return alist

def loadFile(path):
    try:
        handle = open(path, 'r')
        asc = []
        for line in handle:
            line = line.rstrip()
            asc.append(line)
        handle.close()

        return asc
    except:
        error = 'ERROR *** Could not load ' + path
        sys.exit(error)

def MolDynText(fname,Verbose,Plot,Save):
    path = fname
    base = os.path.basename(path)
    fname = os.path.splitext(base)[0]

    if not os.path.isfile(path):
        path = FileFinder.getFullPath(path)

    if Verbose:
        logger.notice('Reading file : ' + path)

    asc = loadFile(path)
    lasc = len(asc)

    val = SplitLine(asc[3])
    Q = []
    for n in range(1,len(val)):
        Q.append(val[n])
    nQ = len(Q)
    x = []
    y = []
    for n in range(4,lasc):
        val = SplitLine(asc[n])
        x.append(val[0])
        yval = val[1:]
        y.append(yval)
    nX = len(x)
    if Verbose:
        logger.notice('nQ = '+str(nQ))
        logger.notice('nT = '+str(nX))
    xT = np.array(x)
    eZero = np.zeros(nX)
    Qaxis = ''
    for m in range(0,nQ):
        if Verbose:
            logger.notice('Q['+str(m+1)+'] : '+str(Q[m]))
        S = []
        for n in range(0,nX):
            S.append(y[n][m])
        if m == 0:
            Qaxis += str(Q[m])
            xDat = xT
            yDat = np.array(S)
            eDat = eZero
        else:
            Qaxis += ','+str(Q[m])
            xDat = np.append(xDat,xT)
            yDat = np.append(yDat,np.array(S))
            eDat = np.append(eDat,eZero)
    outWS = fname + '_iqt'
    CreateWorkspace(OutputWorkspace=outWS, DataX=xDat, DataY=yDat, DataE=eDat,
        Nspec=nQ, UnitX='TOF')
#        Nspec=nQ, UnitX='TOF', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
    Qmax = Q[nQ-1]
    instr = 'MolDyn'
    ana = 'qmax'
    if Qmax <= 2.0:
        refl = '2'
    else:
        refl = '4'
    InstrParas(outWS,instr,ana,refl)
    efixed = RunParas(outWS,instr,fname,fname,Verbose)
    if Verbose:
        logger.notice('Qmax = '+str(Qmax)+' ; efixed = '+str(efixed))
    pi4 = 4.0*math.pi
    wave=1.8*math.sqrt(25.2429/efixed)
    theta = []
    for n in range(0,nQ):
        qw = wave*Q[n]/pi4
        ang = 2.0*math.degrees(math.asin(qw))
        theta.append(ang)
    ChangeAngles(outWS,instr,theta,Verbose)


class MolDyn(PythonAlgorithm):


    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic'


    def PyInit(self):
        self.declareProperty(FileProperty('SampleFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=[".cdl"]),
                                          doc='File path for data')

        self.declareProperty(StringArrayProperty('Functions'),
                             doc='The Function to use')

        self.declareProperty(name='Convolution', defaultValue=False,
                             doc='Perform convolution')

        self.declareProperty(WorkspaceProperty('Resolution', '', Direction.Input, PropertyMode.Optional),
                             doc='Resolution workspace')

        self.declareProperty(name='Crop', defaultValue=False,
                             doc='Crop the energy range')

        self.declareProperty(name='MaxEnergy', defaultValue='',
                             doc='Maximum energy for cropping')

        self.declareProperty(name='Verbose', defaultValue=False,
                             doc='Output more verbose message to log')

        self.declareProperty(name='Plot', defaultValue='None',
                             validator=StringListValidator(['None', 'Spectra', 'Contour', 'Both']),
                             doc='Plot result workspace')

        self.declareProperty(name='Save', defaultValue=False,
                             doc='Save result workspace to nexus file in the default save directory')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc='Output workspace name')


    def PyExec(self):
        from IndirectImport import import_mantidplot
        mtd_plot = import_mantidplot()

        self._setup()

        self._mol_dyn_import(self._sam_path)

        if self._convolve:
            self._convolve_with_res()

        if self._save:
            workdir = config['defaultsave.directory']
            out_filename = os.path.join(workdir, self._out_ws + '.nxs')
            if self._verbose:
                logger.notice('Creating file : ' + out_filename)
                SaveNexus(InputWorkspace=self._out_ws, Filename=out_filename)

        self.setProperty('OutputWorkspace', self._out_ws)

        if self._plot == 'Spectra' or self._plot == 'Both':
            pass
            # nHist = mtd[self._out_ws].getNumberHistograms()
            # if nHist > 10:
            #     nHist = 10
            # plot_list = []
            # for i in range(0, nHist):
            #     plot_list.append(i)

            # mtd_plot.plotSpectrum(self._out_ws, plot_list)

        if self._plot == 'Contour' or self._plot == 'Both':
            mtd_plot.plot2D(self._out_ws)


    def _setup(self):
        self._verbose = self.getProperty('Verbose').value
        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

        self._sam_path = self.getPropertyValue('SampleFile')
        self._functions = self.getProperty('Functions').value

        if self._verbose:
            logger.notice('Function : ' + str(self._functions))

        self._convolve = self.getProperty('Convolution').value
        self._res_ws = self.getPropertyValue('Resolution')

        self._crop = self.getProperty('Crop').value
        self._emax = self.getPropertyValue('MaxEnergy')

        self._out_ws = self.getPropertyValue('OutputWorkspace')


    def _convolve_with_res(self, workspace):
        base = os.path.basename(self._sam_path)
        self._sam_ws = os.path.splitext(base)[0]
        self._sam_ws += '_' + str(self._functions[0])

        f1 = 'composite=Convolution;'
        f2 = 'name=TabulatedFunction,Workspace=' + self._res_ws + ',WorkspaceIndex=0;'
        f3 = 'name=TabulatedFunction,Workspace=' + self._sam_ws + ',WorkspaceIndex=0'
        function = f1 + f2 + f3

        Fit(Function=function, InputWorkspace=self._sam_ws, MaxIterations=0, CreateOutput=True,
            ConvolveMembers=True)

        conv_ws = self._sam_ws + '_conv'
        Symmetrise(Sample=self._sam_ws, XMin=0, XMax=self._emax, Verbose=self._verbose, Plot=False,
                   OutputWorkspace=conv_ws)


    def _mol_dyn_import(self, fname):
        # seperate functions string and strip whitespace
        functions = [x.strip() for x in self._functions]

        path = fname
        base = os.path.basename(path)
        fname = os.path.splitext(base)[0]

        if not os.path.isfile(path):
            path = FileFinder.getFullPath(path)

        asc = loadFile(path)
        lasc = len(asc)

        # raw head
        nQ, nT, nF = FindDimensions(asc, self._verbose)
        ldata = FindStarts(asc, 'data:', 0)
        lq1 = FindStarts(asc, ' q =', ldata)  # start Q values
        lq2 = FindStarts(asc, ' q =', lq1 - 1)
        Qlist = MakeList(asc, lq1, lq2)
        if nQ != len(Qlist):
            raise RUntimeError('Error reading Q values')
        Qf = Qlist[0].split()
        Q = [float(Qf[2]) / 10.0]
        for m in range(1, nQ - 1):
            Q.append(float(Qlist[m]) / 10.0)

        Q.append(float(Qlist[nQ - 1][:-1]) / 10.0)
        if self._verbose:
            logger.notice('Q values = ' + str(Q))

        lt1 = FindStarts(asc, ' time =', lq2)  # start T values
        lt2 = FindEnds(asc, ';', lt1)
        Tlist = MakeList(asc, lt1, lt2)
        if nT != len(Tlist):
            raise RuntimeError('Error reading Time values')

        Tf = Tlist[0].split()
        T = [float(Tf[2])]
        for m in range(1, nT - 1):
            T.append(float(Tlist[m]))

        T.append(float(Tlist[nT - 1][:-1]))
        T.append(2 * T[nT - 1] - T[nT - 2])
        if self._verbose:
            logger.notice('T values = ' + str(T[:2]) + ' to ' + str(T[-3:]))

        lf1 = FindStarts(asc, ' frequency =', lq2)  # start F values
        lf2 = FindEnds(asc, ';', lf1)
        Flist = MakeList(asc, lf1, lf2)
        if nF != len(Flist):
            raise RuntimeError('Error reading Freq values')

        Ff = Flist[0].split()
        F = [float(Ff[2])]
        for m in range(1, nF - 1):
            F.append(float(Flist[m]))

        F.append(float(Flist[nF - 1][:-1]))
        F.append(2 * F[nF - 1] - T[nF - 2])
        if self._verbose:
            logger.notice('F values = ' + str(F[:2]) + ' to ' + str(F[-3:]))

        # Function
        output_ws_list = list()
        for func in functions:
            start = []
            lstart = lt2
            if func[:3] == 'Fqt':
                nP = nT
                xEn = np.array(T)
                eZero = np.zeros(nT)
                xUnit = 'TOF'
            elif func[:3] == 'Sqw':
                nP = nF
                xEn = np.array(F)
                eZero = np.zeros(nF)
                xUnit = 'Energy'
            else:
                raise RuntimeError('Failed to parse function string ' + func)

            for n in range(0, nQ):
                for m in range(lstart, lasc):
                    char = asc[m]
                    if char.startswith('  // ' + func):
                        start.append(m)
                        lstart = m + 1
            lend = FindEnds(asc, ';', lstart)
            start.append(lend + 1)

            # Throw error if we couldn't find the function
            if len(start) < 2:
                raise RuntimeError('Failed to parse function string ' + func)

            Qaxis = ''
            for n in range(0, nQ):
                if self._verbose:
                    logger.information(str(start))
                    logger.notice('Reading : ' + asc[start[n]])

                Slist = MakeList(asc, start[n] + 1, start[n + 1] - 1)
                if n == nQ - 1:
                    Slist[nP - 1] = Slist[nP - 1][:-1]
                S = []
                for m in range(0, nP):
                    S.append(float(Slist[m]))
                if nP != len(S):
                    raise RuntimeError('Error reading S values')
                else:
                    if self._verbose:
                        logger.notice('S values = ' + str(S[:2]) + ' to ' + str(S[-2:]))
                if n == 0:
                    Qaxis += str(Q[n])
                    xDat = xEn
                    yDat = np.array(S)
                    eDat = eZero
                else:
                    Qaxis += ',' + str(Q[n])
                    xDat = np.append(xDat, xEn)
                    yDat = np.append(yDat, np.array(S))
                    eDat = np.append(eDat, eZero)
            outWS = fname + '_' + func
            CreateWorkspace(OutputWorkspace=outWS, DataX=xDat, DataY=yDat, DataE=eDat,
                            Nspec=nQ, UnitX=xUnit, VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
            output_ws_list.append(outWS)

        GroupWorkspaces(InputWorkspaces=output_ws_list, OutputWorkspace=self._out_ws)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(MolDyn)
