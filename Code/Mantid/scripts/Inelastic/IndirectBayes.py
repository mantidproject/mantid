#pylint: disable=invalid-name
# Bayes routines
# Fortran programs use fixed length arrays whereas Python has variable lenght lists
# Input : the Python list is padded to Fortrans length using procedure PadArray
# Output : the Fortran numpy array is sliced to Python length using dataY = yout[:ny]
#
from IndirectImport import *
if is_supported_f2py_platform():
    QLr     = import_f2py("QLres")
    QLd     = import_f2py("QLdata")
    Qse     = import_f2py("QLse")
    Que     = import_f2py("Quest")
    resnorm = import_f2py("ResNorm")
else:
    unsupported_message()

from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import *
import sys, platform, math, os.path, numpy as np
mp = import_mantidplot()

def readASCIIFile(file_name):
    workdir = config['defaultsave.directory']

    file_path = os.path.join(workdir, file_name)
    asc = []

    with open(file_path, 'r') as handle:
        for line in handle:
            line = line.rstrip()
            asc.append(line)

    return asc

def CalcErange(inWS,ns,erange,binWidth):
    #length of array in Fortran
    array_len = 4096

    binWidth = int(binWidth)
    bnorm = 1.0/binWidth

    #get data from input workspace
    N,X,Y,E = GetXYE(inWS,ns,array_len)
    Xdata = mtd[inWS].readX(0)

    #get all x values within the energy range
    rangeMask = (Xdata >= erange[0]) & (Xdata <= erange[1])
    Xin = Xdata[rangeMask]

    #get indicies of the bounds of our energy range
    minIndex = np.where(Xdata==Xin[0])[0][0]+1
    maxIndex = np.where(Xdata==Xin[-1])[0][0]

    #reshape array into sublists of bins
    Xin = Xin.reshape(len(Xin)/binWidth, binWidth)

    #sum and normalise values in bins
    Xout = [sum(bin)*bnorm for bin in Xin]

    #count number of bins
    nbins = len(Xout)

    nout = [nbins, minIndex, maxIndex]

     #pad array for use in Fortran code
    Xout = PadArray(Xout,array_len)

    return nout,bnorm,Xout,X,Y,E

def GetXYE(inWS,n,array_len):
    Xin = mtd[inWS].readX(n)
    N = len(Xin)-1                            # get no. points from length of x array
    Yin = mtd[inWS].readY(n)
    Ein = mtd[inWS].readE(n)
    X=PadArray(Xin,array_len)
    Y=PadArray(Yin,array_len)
    E=PadArray(Ein,array_len)
    return N,X,Y,E

def GetResNorm(resnormWS,ngrp):
    if ngrp == 0:                                # read values from WS
        dtnorm = mtd[resnormWS+'_Intensity'].readY(0)
        xscale = mtd[resnormWS+'_Stretch'].readY(0)
    else:                                        # constant values
        dtnorm = []
        xscale = []
        for m in range(0,ngrp):
            dtnorm.append(1.0)
            xscale.append(1.0)
    dtn=PadArray(dtnorm,51)                      # pad for Fortran call
    xsc=PadArray(xscale,51)
    return dtn,xsc

def ReadNormFile(readRes,resnormWS,nsam):            # get norm & scale values
    if readRes:                   # use ResNorm file option=o_res
        Xin = mtd[resnormWS+'_Intensity'].readX(0)
        nrm = len(Xin)                        # no. points from length of x array
        if nrm == 0:
            raise ValueError('ResNorm file has no Intensity points')
        Xin = mtd[resnormWS+'_Stretch'].readX(0)                    # no. points from length of x array
        if len(Xin) == 0:
            raise ValueError('ResNorm file has no xscale points')
        if nrm != nsam:                # check that no. groups are the same
            raise ValueError('ResNorm groups (' +str(nrm) + ') not = Sample (' +str(nsam) +')')
        else:
            dtn,xsc = GetResNorm(resnormWS,0)
    else:
        # do not use ResNorm file
        dtn,xsc = GetResNorm(resnormWS,nsam)
    return dtn,xsc

#Reads in a width ASCII file
def ReadWidthFile(readWidth,widthFile,numSampleGroups):
    widthY = []
    widthE = []

    if readWidth:

        logger.information('Width file is ' + widthFile)

        # read ascii based width file
        try:
            wfPath = FileFinder.getFullPath(widthFile)
            handle = open(wfPath, 'r')
            asc = []

            for line in handle:
                line = line.rstrip()
                asc.append(line)
            handle.close()

        except Exception, e:
            raise ValueError('Failed to read width file')

        numLines = len(asc)

        if numLines == 0:
            raise ValueError('No groups in width file')

        if numLines != numSampleGroups:                # check that no. groups are the same
            raise ValueError('Width groups (' +str(numLines) + ') not = Sample (' +str(numSampleGroups) +')')
    else:
         # no file: just use constant values
        widthY = np.zeros(numSampleGroups)
        widthE = np.zeros(numSampleGroups)

    # pad for Fortran call
    widthY = PadArray(widthY,51)
    widthE = PadArray(widthE,51)

    return widthY, widthE

# QLines programs
def QLRun(program,samWS,resWS,resnormWS,erange,nbins,Fit,wfile,Loop,Plot,Save):
    StartTime(program)

    #expand fit options
    elastic, background, width, resnorm = Fit

    #convert true/false to 1/0 for fortran
    o_el = 1 if elastic else 0
    o_w1 = 1 if width else 0
    o_res = 1 if resnorm else 0

    #fortran code uses background choices defined using the following numbers
    if background == 'Sloping':
        o_bgd = 2
    elif background == 'Flat':
        o_bgd = 1
    elif background == 'Zero':
        o_bgd = 0

    fitOp = [o_el, o_bgd, o_w1, o_res]

    workdir = getDefaultWorkingDirectory()

    facility = config['default.facility']
    array_len = 4096                           # length of array in Fortran
    CheckXrange(erange,'Energy')

    nbin,nrbin = nbins[0], nbins[1]

    logger.information('Sample is ' + samWS)
    logger.information('Resolution is ' + resWS)

    CheckAnalysers(samWS,resWS)
    efix = getEfixed(samWS)
    theta, Q = GetThetaQ(samWS)

    nsam,ntc = CheckHistZero(samWS)

    totalNoSam = nsam

    #check if we're performing a sequential fit
    if Loop != True:
        nsam = 1

    nres,ntr = CheckHistZero(resWS)

    if program == 'QL':
        if nres == 1:
            prog = 'QLr'                        # res file
        else:
            prog = 'QLd'                        # data file
            CheckHistSame(samWS,'Sample',resWS,'Resolution')
    elif program == 'QSe':
        if nres == 1:
            prog = 'QSe'                        # res file
        else:
            raise ValueError('Stretched Exp ONLY works with RES file')

    logger.information('Version is ' +prog)
    logger.information(' Number of spectra = '+str(nsam))
    logger.information(' Erange : '+str(erange[0])+' to '+str(erange[1]))

    Wy,We = ReadWidthFile(width,wfile,totalNoSam)
    dtn,xsc = ReadNormFile(resnorm,resnormWS,totalNoSam)

    fname = samWS[:-4] + '_'+ prog
    probWS = fname + '_Prob'
    fitWS = fname + '_Fit'
    datWS = fname + '_Data'
    wrks=os.path.join(workdir, samWS[:-4])
    logger.information(' lptfile : '+wrks+'_'+prog+'.lpt')
    lwrk=len(wrks)
    wrks.ljust(140,' ')
    wrkr=resWS
    wrkr.ljust(140,' ')
    wrk = [wrks, wrkr]

    # initialise probability list
    if program == 'QL':
        prob0 = []
        prob1 = []
        prob2 = []
    xQ = np.array([Q[0]])
    for m in range(1,nsam):
        xQ = np.append(xQ,Q[m])
    xProb = xQ
    xProb = np.append(xProb,xQ)
    xProb = np.append(xProb,xQ)
    eProb = np.zeros(3*nsam)

    group = ''
    for m in range(0,nsam):
        logger.information('Group ' +str(m)+ ' at angle '+ str(theta[m]))
        nsp = m+1
        nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(samWS,m,erange,nbin)
        Ndat = nout[0]
        Imin = nout[1]
        Imax = nout[2]
        if prog == 'QLd':
            mm = m
        else:
            mm = 0
        Nb,Xb,Yb,Eb = GetXYE(resWS,mm,array_len)     # get resolution data
        numb = [nsam, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin]
        rscl = 1.0
        reals = [efix, theta[m], rscl, bnorm]

        if prog == 'QLr':
            nd,xout,yout,eout,yfit,yprob=QLr.qlres(numb,Xv,Yv,Ev,reals,fitOp,
                                                   Xdat,Xb,Yb,Wy,We,dtn,xsc,
                                                   wrks,wrkr,lwrk)
            message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])+' '+str(yprob[2])+' '+str(yprob[3])
            logger.information(message)
        if prog == 'QLd':
            nd,xout,yout,eout,yfit,yprob=QLd.qldata(numb,Xv,Yv,Ev,reals,fitOp,
                                                    Xdat,Xb,Yb,Eb,Wy,We,
                                                    wrks,wrkr,lwrk)
            message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])+' '+str(yprob[2])+' '+str(yprob[3])
            logger.information(message)
        if prog == 'QSe':
            nd,xout,yout,eout,yfit,yprob=Qse.qlstexp(numb,Xv,Yv,Ev,reals,fitOp,\
                                                    Xdat,Xb,Yb,Wy,We,dtn,xsc,\
                                                    wrks,wrkr,lwrk)
        dataX = xout[:nd]
        dataX = np.append(dataX,2*xout[nd-1]-xout[nd-2])
        yfit_list = np.split(yfit[:4*nd],4)
        dataF0 = yfit_list[0]
        dataF1 = yfit_list[1]
        if program == 'QL':
            dataF2 = yfit_list[2]
            dataF3 = yfit_list[3]
        dataG = np.zeros(nd)
        datX = dataX
        datY = yout[:nd]
        datE = eout[:nd]
        datX = np.append(datX,dataX)
        datY = np.append(datY,dataF1[:nd])
        datE = np.append(datE,dataG)
        res1 = dataF1[:nd] - yout[:nd]
        datX = np.append(datX,dataX)
        datY = np.append(datY,res1)
        datE = np.append(datE,dataG)
        nsp = 3
        names = 'data,fit.1,diff.1'
        res_plot = [0, 1, 2]
        if program == 'QL':
            datX = np.append(datX,dataX)
            datY = np.append(datY,dataF2[:nd])
            datE = np.append(datE,dataG)
            res2 = dataF2[:nd] - yout[:nd]
            datX = np.append(datX,dataX)
            datY = np.append(datY,res2)
            datE = np.append(datE,dataG)
            nsp += 2
            names += ',fit.2,diff.2'
            res_plot.append(4)
            prob0.append(yprob[0])
            prob1.append(yprob[1])
            prob2.append(yprob[2])

        # create result workspace
        fitWS = fname+'_Workspaces'
        fout = fname+'_Workspace_'+ str(m)

        CreateWorkspace(OutputWorkspace=fout, DataX=datX, DataY=datY, DataE=datE,\
            Nspec=nsp, UnitX='DeltaE', VerticalAxisUnit='Text', VerticalAxisValues=names)

        # append workspace to list of results
        group += fout + ','

    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS)

    if program == 'QL':
        yPr0 = np.array([prob0[0]])
        yPr1 = np.array([prob1[0]])
        yPr2 = np.array([prob2[0]])
        for m in range(1,nsam):
            yPr0 = np.append(yPr0,prob0[m])
            yPr1 = np.append(yPr1,prob1[m])
            yPr2 = np.append(yPr2,prob2[m])
        yProb = yPr0
        yProb = np.append(yProb,yPr1)
        yProb = np.append(yProb,yPr2)
        CreateWorkspace(OutputWorkspace=probWS, DataX=xProb, DataY=yProb, DataE=eProb,\
            Nspec=3, UnitX='MomentumTransfer')
        outWS = C2Fw(samWS[:-4],fname)
        if (Plot != 'None'):
            QuasiPlot(fname,Plot,res_plot,Loop)
    if program == 'QSe':
        outWS = C2Se(fname)
        if (Plot != 'None'):
            QuasiPlot(fname,Plot,res_plot,Loop)

    #Add some sample logs to the output workspaces
    CopyLogs(InputWorkspace=samWS, OutputWorkspace=outWS)
    QLAddSampleLogs(outWS, resWS, prog, background, elastic, erange, (nbin, nrbin), resnormWS, wfile)
    CopyLogs(InputWorkspace=samWS, OutputWorkspace=fitWS)
    QLAddSampleLogs(fitWS, resWS, prog, background, elastic, erange, (nbin, nrbin), resnormWS, wfile)

    if Save:
        fit_path = os.path.join(workdir,fitWS+'.nxs')
        SaveNexusProcessed(InputWorkspace=fitWS, Filename=fit_path)
        out_path = os.path.join(workdir, outWS+'.nxs')                    # path name for nxs file
        SaveNexusProcessed(InputWorkspace=outWS, Filename=out_path)
        logger.information('Output fit file created : ' + fit_path)
        logger.information('Output paramter file created : ' + out_path)

    EndTime(program)

def QLAddSampleLogs(workspace, res_workspace, fit_program, background, elastic_peak, e_range, binning, resnorm_workspace, width_file):

    sample_binning, res_binning = binning
    energy_min, energy_max = e_range

    AddSampleLog(Workspace=workspace, LogName="res_file", LogType="String", LogText=res_workspace)
    AddSampleLog(Workspace=workspace, LogName="fit_program", LogType="String", LogText=fit_program)
    AddSampleLog(Workspace=workspace, LogName="background", LogType="String", LogText=str(background))
    AddSampleLog(Workspace=workspace, LogName="elastic_peak", LogType="String", LogText=str(elastic_peak))
    AddSampleLog(Workspace=workspace, LogName="energy_min", LogType="Number", LogText=str(energy_min))
    AddSampleLog(Workspace=workspace, LogName="energy_max", LogType="Number", LogText=str(energy_max))
    AddSampleLog(Workspace=workspace, LogName="sample_binning", LogType="Number", LogText=str(sample_binning))
    AddSampleLog(Workspace=workspace, LogName="resolution_binning", LogType="Number", LogText=str(res_binning))

    resnorm_used = (resnorm_workspace != '')
    AddSampleLog(Workspace=workspace, LogName="resnorm", LogType="String", LogText=str(resnorm_used))
    if resnorm_used:
        AddSampleLog(Workspace=workspace, LogName="resnorm_file", LogType="String", LogText=resnorm_workspace)

    width_file_used = (width_file != '')
    AddSampleLog(Workspace=workspace, LogName="width", LogType="String", LogText=str(width_file_used))
    if width_file_used:
        AddSampleLog(Workspace=workspace, LogName="width_file", LogType="String", LogText=width_file)

def yield_floats(block):
    #yield a list of floats from a list of lines of text
    #encapsulates the iteration over a block of lines
    for line in block:
        yield ExtractFloat(line)

def read_ql_file(file_name, nl):
    #offet to ignore header
    header_offset = 8
    block_size = 4+nl*3

    asc = readASCIIFile(file_name)
    #extract number of blocks from the file header
    num_blocks = int(ExtractFloat(asc[3])[0])

    q_data = []
    amp_data, FWHM_data, height_data = [], [], []
    amp_error, FWHM_error, height_error = [], [], []

    #iterate over each block of fit parameters in the file
    #each block corresponds to a single column in the final workspace
    for block_num in xrange(num_blocks):
        lower_index = header_offset+(block_size*block_num)
        upper_index = lower_index+block_size

        #create iterator for each line in the block
        line_pointer = yield_floats(asc[lower_index:upper_index])

        #Q,AMAX,HWHM,BSCL,GSCL
        line = line_pointer.next()
        Q, AMAX, HWHM, BSCL, GSCL = line
        q_data.append(Q)

        #A0,A1,A2,A4
        line = line_pointer.next()
        block_height = AMAX*line[0]

        #parse peak data from block
        block_FWHM = []
        block_amplitude = []
        for i in range(nl):
            #Amplitude,FWHM for each peak
            line = line_pointer.next()
            amp = AMAX*line[0]
            FWHM = 2.*HWHM*line[1]
            block_amplitude.append(amp)
            block_FWHM.append(FWHM)

        #next parse error data from block
        #SIG0
        line = line_pointer.next()
        block_height_e = line[0]

        block_FWHM_e = []
        block_amplitude_e = []
        for i in range(nl):
            #Amplitude error,FWHM error for each peak
            #SIGIK
            line = line_pointer.next()
            amp = AMAX*math.sqrt(math.fabs(line[0])+1.0e-20)
            block_amplitude_e.append(amp)

            #SIGFK
            line = line_pointer.next()
            FWHM = 2.0*HWHM*math.sqrt(math.fabs(line[0])+1.0e-20)
            block_FWHM_e.append(FWHM)

        #append data from block
        amp_data.append(block_amplitude)
        FWHM_data.append(block_FWHM)
        height_data.append(block_height)

        #append error values from block
        amp_error.append(block_amplitude_e)
        FWHM_error.append(block_FWHM_e)
        height_error.append(block_height_e)

    return q_data, (amp_data, FWHM_data, height_data), (amp_error, FWHM_error, height_error)

def C2Fw(prog,sname):
    output_workspace = sname+'_Result'
    num_spectra = 0

    axis_names = []
    x, y, e = [], [], []
    for nl in range(1,4):
        num_params = nl*3+1
        num_spectra += num_params

        amplitude_data, width_data = [], []
        amplitude_error, width_error  = [], []

        #read data from file output by fortran code
        file_name = sname + '.ql' +str(nl)
        x_data, peak_data, peak_error = read_ql_file(file_name, nl)
        x_data = np.asarray(x_data)

        amplitude_data, width_data, height_data = peak_data
        amplitude_error, width_error, height_error = peak_error

        #transpose y and e data into workspace rows
        amplitude_data, width_data = np.asarray(amplitude_data).T, np.asarray(width_data).T
        amplitude_error, width_error = np.asarray(amplitude_error).T, np.asarray(width_error).T
        height_data, height_error = np.asarray(height_data), np.asarray(height_error)

        #calculate EISF and EISF error
        total = height_data+amplitude_data
        EISF_data = height_data / total
        total_error = height_error**2 + amplitude_error**2
        EISF_error = EISF_data * np.sqrt((height_error**2/height_data**2) + (total_error/total**2))

        #interlace amplitudes and widths of the peaks
        y.append(np.asarray(height_data))
        for amp, width, EISF in zip(amplitude_data, width_data, EISF_data):
            y.append(amp)
            y.append(width)
            y.append(EISF)

        #iterlace amplitude and width errors of the peaks
        e.append(np.asarray(height_error))
        for amp, width, EISF in zip(amplitude_error, width_error, EISF_error):
            e.append(amp)
            e.append(width)
            e.append(EISF)

        #create x data and axis names for each function
        axis_names.append('f'+str(nl)+'.f0.'+'Height')
        x.append(x_data)
        for j in range(1,nl+1):
            axis_names.append('f'+str(nl)+'.f'+str(j)+'.Amplitude')
            x.append(x_data)
            axis_names.append('f'+str(nl)+'.f'+str(j)+'.FWHM')
            x.append(x_data)
            axis_names.append('f'+str(nl)+'.f'+str(j)+'.EISF')
            x.append(x_data)

    x = np.asarray(x).flatten()
    y = np.asarray(y).flatten()
    e = np.asarray(e).flatten()

    CreateWorkspace(OutputWorkspace=output_workspace, DataX=x, DataY=y, DataE=e, Nspec=num_spectra,\
        UnitX='MomentumTransfer', YUnitLabel='', VerticalAxisUnit='Text', VerticalAxisValues=axis_names)

    return output_workspace

def SeBlock(a,first):                                 #read Ascii block of Integers
    line1 = a[first]
    first += 1
    val = ExtractFloat(a[first])               #Q,AMAX,HWHM
    Q = val[0]
    AMAX = val[1]
    HWHM = val[2]
    first += 1
    val = ExtractFloat(a[first])               #A0
    int0 = [AMAX*val[0]]
    first += 1
    val = ExtractFloat(a[first])                #AI,FWHM first peak
    fw = [2.*HWHM*val[1]]
    int = [AMAX*val[0]]
    first += 1
    val = ExtractFloat(a[first])                 #SIG0
    int0.append(val[0])
    first += 1
    val = ExtractFloat(a[first])                  #SIG3K
    int.append(AMAX*math.sqrt(math.fabs(val[0])+1.0e-20))
    first += 1
    val = ExtractFloat(a[first])                  #SIG1K
    fw.append(2.0*HWHM*math.sqrt(math.fabs(val[0])+1.0e-20))
    first += 1
    be = ExtractFloat(a[first])                  #EXPBET
    first += 1
    val = ExtractFloat(a[first])                  #SIG2K
    be.append(math.sqrt(math.fabs(val[0])+1.0e-20))
    first += 1
    return first,Q,int0,fw,int,be                                      #values as list

def C2Se(sname):
    prog = 'QSe'
    outWS = sname+'_Result'
    asc = readASCIIFile(sname+'.qse')
    lasc = len(asc)
    var = asc[3].split()                            #split line on spaces
    nspec = var[0]
    ndat = var[1]
    var = ExtractInt(asc[6])
    first = 7
    Xout = []
    Yf = []
    Ef = []
    Yi = []
    Ei = []
    Yb = []
    Eb = []
    ns = int(nspec)

    dataX = np.array([])
    dataY = np.array([])
    dataE = np.array([])

    for m in range(0,ns):
        first,Q,int0,fw,it,be = SeBlock(asc,first)
        Xout.append(Q)
        Yf.append(fw[0])
        Ef.append(fw[1])
        Yi.append(it[0])
        Ei.append(it[1])
        Yb.append(be[0])
        Eb.append(be[1])
    Vaxis = []

    dataX = np.append(dataX,np.array(Xout))
    dataY = np.append(dataY,np.array(Yi))
    dataE = np.append(dataE,np.array(Ei))
    nhist = 1
    Vaxis.append('f1.Amplitude')

    dataX = np.append(dataX, np.array(Xout))
    dataY = np.append(dataY, np.array(Yf))
    dataE = np.append(dataE, np.array(Ef))
    nhist += 1
    Vaxis.append('f1.FWHM')

    dataX = np.append(dataX,np.array(Xout))
    dataY = np.append(dataY,np.array(Yb))
    dataE = np.append(dataE,np.array(Eb))
    nhist += 1
    Vaxis.append('f1.Beta')

    logger.information('Vaxis=' + str(Vaxis))
    CreateWorkspace(OutputWorkspace=outWS, DataX=dataX, DataY=dataY, DataE=dataE, Nspec=nhist,\
        UnitX='MomentumTransfer', VerticalAxisUnit='Text', VerticalAxisValues=Vaxis, YUnitLabel='')
    return outWS

def QuasiPlot(ws_stem,plot_type,res_plot,sequential):
    if plot_type:
        if sequential:
            ws_name = ws_stem + '_Result'
            num_spectra = mtd[ws_name].getNumberHistograms()

            if (plot_type == 'Prob' or plot_type == 'All'):
                prob_ws = ws_stem+'_Prob'
                if prob_ws in mtd.getObjectNames():
                    mp.plotSpectrum(prob_ws,[1,2],False)

            QuasiPlotParameters(ws_name, plot_type)

        if (plot_type == 'Fit' or plot_type == 'All'):
            fWS = ws_stem+'_Workspace_0'
            f_plot=mp.plotSpectrum(fWS,res_plot,False)


def QuasiPlotParameters(ws_name, plot_type):
    """
    Plot a parameter if the user requested it and it exists
    in the workspace

    @param ws_name :: name of the workspace to plot from. This function expects it has a TextAxis
    @param plot_type :: the name of the parameter to plot (or All if all parameters should be plotted)
    """
    num_spectra = mtd[ws_name].getNumberHistograms()
    param_names = ['Amplitude', 'FWHM', 'Beta']

    for param_name in param_names:
        if (plot_type == param_name or plot_type == 'All'):
            spectra_indicies = [i for i in range(num_spectra) if param_name in mtd[ws_name].getAxis(1).label(i)]

            if(len(spectra_indicies) > 0):
                plotSpectra(ws_name, param_name, indicies=spectra_indicies[:3])

# Quest programs
def CheckBetSig(nbs):
    Nsig = int(nbs[1])
    if Nsig == 0:
        raise ValueError('Number of sigma points is Zero')
    if Nsig > 200:
        raise ValueError('Max number of sigma points is 200')

    Nbet = int(nbs[0])
    if Nbet == 0:
        raise ValueError('Number of beta points is Zero')
    if Nbet > 200:
        raise ValueError('Max number of beta points is 200')

    return Nbet,Nsig

def QuestRun(samWS,resWS,nbs,erange,nbins,Fit,Loop,Plot,Save):
    StartTime('Quest')
    #expand fit options
    elastic, background, width, resnorm = Fit

    #convert true/false to 1/0 for fortran
    o_el = 1 if elastic else 0
    o_w1 = 1 if width else 0
    o_res = 1 if resnorm else 0

    #fortran code uses background choices defined using the following numbers
    if background == 'Sloping':
        o_bgd = 2
    elif background == 'Flat':
        o_bgd = 1
    elif background == 'Zero':
        o_bgd = 0

    fitOp = [o_el, o_bgd, o_w1, o_res]

    workdir = getDefaultWorkingDirectory()

    array_len = 4096                           # length of array in Fortran
    CheckXrange(erange,'Energy')
    nbin,nrbin = nbins[0],nbins[1]
    logger.information('Sample is ' + samWS)
    logger.information('Resolution is ' + resWS)
    CheckAnalysers(samWS,resWS)
    nsam,ntc = CheckHistZero(samWS)

    if Loop != True:
        nsam = 1

    efix = getEfixed(samWS)
    theta,Q = GetThetaQ(samWS)
    nres,ntr = CheckHistZero(resWS)
    if nres == 1:
        prog = 'Qst'                        # res file
    else:
        raise ValueError('Stretched Exp ONLY works with RES file')
    logger.information(' Number of spectra = '+str(nsam))
    logger.information(' Erange : '+str(erange[0])+' to '+str(erange[1]))

    fname = samWS[:-4] + '_'+ prog
    wrks=os.path.join(workdir, samWS[:-4])
    logger.information(' lptfile : ' + wrks +'_Qst.lpt')
    lwrk=len(wrks)
    wrks.ljust(140,' ')
    wrkr=resWS
    wrkr.ljust(140,' ')
    wrk = [wrks, wrkr]
    Nbet,Nsig = nbs[0], nbs[1]
    eBet0 = np.zeros(Nbet)                  # set errors to zero
    eSig0 = np.zeros(Nsig)                  # set errors to zero
    rscl = 1.0
    Qaxis = ''
    for m in range(0,nsam):
        logger.information('Group ' +str(m)+ ' at angle '+ str(theta[m]))
        nsp = m+1
        nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(samWS,m,erange,nbin)
        Ndat = nout[0]
        Imin = nout[1]
        Imax = nout[2]
        Nb,Xb,Yb,Eb = GetXYE(resWS,0,array_len)
        numb = [nsam, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin, Nbet, Nsig]
        reals = [efix, theta[m], rscl, bnorm]
        xsout,ysout,xbout,ybout,zpout=Que.quest(numb,Xv,Yv,Ev,reals,fitOp,\
                                            Xdat,Xb,Yb,wrks,wrkr,lwrk)
        dataXs = xsout[:Nsig]               # reduce from fixed Fortran array
        dataYs = ysout[:Nsig]
        dataXb = xbout[:Nbet]
        dataYb = ybout[:Nbet]
        zpWS = fname + '_Zp' +str(m)
        if (m > 0):
            Qaxis += ','
        Qaxis += str(Q[m])

        dataXz = []
        dataYz = []
        dataEz = []

        for n in range(0,Nsig):
            yfit_list = np.split(zpout[:Nsig*Nbet],Nsig)
            dataYzp = yfit_list[n]

            dataXz = np.append(dataXz,xbout[:Nbet])
            dataYz = np.append(dataYz,dataYzp[:Nbet])
            dataEz = np.append(dataEz,eBet0)

        CreateWorkspace(OutputWorkspace=zpWS, DataX=dataXz, DataY=dataYz, DataE=dataEz,\
            Nspec=Nsig, UnitX='MomentumTransfer', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=dataXs)

        unitx = mtd[zpWS].getAxis(0).setUnit("Label")
        unitx.setLabel('beta' , '')
        unity = mtd[zpWS].getAxis(1).setUnit("Label")
        unity.setLabel('sigma' , '')

        if m == 0:
            xSig = dataXs
            ySig = dataYs
            eSig = eSig0
            xBet = dataXb
            yBet = dataYb
            eBet = eBet0
            groupZ = zpWS
        else:
            xSig = np.append(xSig,dataXs)
            ySig = np.append(ySig,dataYs)
            eSig = np.append(eSig,eSig0)
            xBet = np.append(xBet,dataXb)
            yBet = np.append(yBet,dataYb)
            eBet = np.append(eBet,eBet0)
            groupZ = groupZ +','+ zpWS

    #create workspaces for sigma and beta
    CreateWorkspace(OutputWorkspace=fname+'_Sigma', DataX=xSig, DataY=ySig, DataE=eSig,\
        Nspec=nsam, UnitX='', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
    unitx = mtd[fname+'_Sigma'].getAxis(0).setUnit("Label")
    unitx.setLabel('sigma' , '')

    CreateWorkspace(OutputWorkspace=fname+'_Beta', DataX=xBet, DataY=yBet, DataE=eBet,\
        Nspec=nsam, UnitX='', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
    unitx = mtd[fname+'_Beta'].getAxis(0).setUnit("Label")
    unitx.setLabel('beta' , '')

    group = fname + '_Sigma,'+ fname + '_Beta'

    fit_workspace = fname+'_Fit'
    contour_workspace = fname+'_Contour'
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fit_workspace)
    GroupWorkspaces(InputWorkspaces=groupZ,OutputWorkspace=contour_workspace)

    #add sample logs to the output workspaces
    CopyLogs(InputWorkspace=samWS, OutputWorkspace=fit_workspace)
    QuestAddSampleLogs(fit_workspace, resWS, background, elastic, erange, nbin, Nsig, Nbet)
    CopyLogs(InputWorkspace=samWS, OutputWorkspace=contour_workspace)
    QuestAddSampleLogs(contour_workspace, resWS, background, elastic, erange, nbin, Nsig, Nbet)

    if Save:
        fpath = os.path.join(workdir,fit_workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=fit_workspace, Filename=fpath)

        cpath = os.path.join(workdir,contour_workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=contour_workspace, Filename=cpath)

        logger.information('Output file for Fit : ' + fpath)
        logger.information('Output file for Contours : ' + cpath)

    if (Plot != 'None' and Loop == True):
        QuestPlot(fname,Plot)
    EndTime('Quest')

def QuestAddSampleLogs(workspace, res_workspace, background, elastic_peak, e_range, sample_binning, sigma, beta):
    energy_min, energy_max = e_range

    AddSampleLog(Workspace=workspace, LogName="res_file", LogType="String", LogText=res_workspace)
    AddSampleLog(Workspace=workspace, LogName="background", LogType="String", LogText=str(background))
    AddSampleLog(Workspace=workspace, LogName="elastic_peak", LogType="String", LogText=str(elastic_peak))
    AddSampleLog(Workspace=workspace, LogName="energy_min", LogType="Number", LogText=str(energy_min))
    AddSampleLog(Workspace=workspace, LogName="energy_max", LogType="Number", LogText=str(energy_max))
    AddSampleLog(Workspace=workspace, LogName="sample_binning", LogType="Number", LogText=str(sample_binning))
    AddSampleLog(Workspace=workspace, LogName="sigma", LogType="Number", LogText=str(sigma))
    AddSampleLog(Workspace=workspace, LogName="beta", LogType="Number", LogText=str(beta))


def QuestPlot(inputWS,Plot):
    if (Plot == 'Sigma' or Plot == 'All'):
        sig_plot=mp.importMatrixWorkspace(inputWS+'_Sigma').plotGraph2D()
    if (Plot == 'Beta' or Plot == 'All'):
        beta_plot=mp.importMatrixWorkspace(inputWS+'_Beta').plotGraph2D()

# ResNorm programs
def ResNormRun(vname,rname,erange,nbin,Plot='None',Save=False):
    StartTime('ResNorm')

    workdir = getDefaultWorkingDirectory()

    array_len = 4096                                    # length of Fortran array
    CheckXrange(erange,'Energy')
    CheckAnalysers(vname,rname)
    nvan,ntc = CheckHistZero(vname)
    theta,Q = GetThetaQ(vname)
    efix = getEfixed(vname)
    nres,ntr = CheckHistZero(rname)
    print "begining erange calc"
    nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(vname,0,erange,nbin)
    print "end of erange calc"
    Ndat = nout[0]
    Imin = nout[1]
    Imax = nout[2]
    wrks=os.path.join(workdir, vname[:-4])
    logger.information(' Number of spectra = '+str(nvan))
    logger.information(' lptfile : ' + wrks +'_resnrm.lpt')
    lwrk=len(wrks)
    wrks.ljust(140,' ')                              # pad for fioxed Fortran length
    wrkr=rname
    wrkr.ljust(140,' ')
    Nb,Xb,Yb,Eb = GetXYE(rname,0,array_len)
    rscl = 1.0
    xPar = np.array([theta[0]])
    for m in range(1,nvan):
        xPar = np.append(xPar,theta[m])
    ePar = np.zeros(nvan)
    fname = vname[:-4]
    for m in range(0,nvan):
        logger.information('Group ' +str(m)+ ' at angle '+ str(theta[m]))
        ntc,Xv,Yv,Ev = GetXYE(vname,m,array_len)
        nsp = m+1
        numb = [nvan, nsp, ntc, Ndat, nbin, Imin, Imax, Nb]
        reals = [efix, theta[0], rscl, bnorm]
        nd,xout,yout,eout,yfit,pfit=resnorm.resnorm(numb,Xv,Yv,Ev,reals,\
                                    Xdat,Xb,Yb,wrks,wrkr,lwrk)
        message = ' Fit paras : '+str(pfit[0])+' '+str(pfit[1])
        logger.information(message)
        dataX = xout[:nd]
        dataX = np.append(dataX,2*xout[nd-1]-xout[nd-2])
        if m == 0:
            yPar1 = np.array([pfit[0]])
            yPar2 = np.array([pfit[1]])
            CreateWorkspace(OutputWorkspace='Data', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],\
                NSpec=1, UnitX='DeltaE')
            CreateWorkspace(OutputWorkspace='Fit', DataX=dataX, DataY=yfit[:nd], DataE=np.zeros(nd),\
                NSpec=1, UnitX='DeltaE')
        else:
            yPar1 = np.append(yPar1,pfit[0])
            yPar2 = np.append(yPar2,pfit[1])
            CreateWorkspace(OutputWorkspace='__datmp', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],\
                NSpec=1, UnitX='DeltaE')
            ConjoinWorkspaces(InputWorkspace1='Data', InputWorkspace2='__datmp', CheckOverlapping=False)
            CreateWorkspace(OutputWorkspace='__f1tmp', DataX=dataX, DataY=yfit[:nd], DataE=np.zeros(nd),\
                NSpec=1, UnitX='DeltaE')
            ConjoinWorkspaces(InputWorkspace1='Fit', InputWorkspace2='__f1tmp', CheckOverlapping=False)

    resnorm_intesity = fname+'_ResNorm_Intensity'
    resnorm_stretch = fname+'_ResNorm_Stretch'

    CreateWorkspace(OutputWorkspace=resnorm_intesity, DataX=xPar, DataY=yPar1, DataE=xPar,\
        NSpec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=resnorm_stretch, DataX=xPar, DataY=yPar2, DataE=xPar,\
        NSpec=1, UnitX='MomentumTransfer')

    group = resnorm_intesity + ','+ resnorm_stretch

    resnorm_workspace = fname+'_ResNorm'
    resnorm_fit_workspace = fname+'_ResNorm_Fit'

    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=resnorm_workspace)
    GroupWorkspaces(InputWorkspaces='Data,Fit',OutputWorkspace=resnorm_fit_workspace)

    CopyLogs(InputWorkspace=vname, OutputWorkspace=resnorm_workspace)
    ResNormAddSampleLogs(resnorm_workspace, erange, nbin)

    CopyLogs(InputWorkspace=vname, OutputWorkspace=resnorm_fit_workspace)
    ResNormAddSampleLogs(resnorm_fit_workspace, erange, nbin)

    if Save:
        par_path = os.path.join(workdir,resnorm_workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=resnorm_workspace, Filename=par_path)

        fit_path = os.path.join(workdir,resnorm_fit_workspace+'.nxs')
        SaveNexusProcessed(InputWorkspace=resnorm_fit_workspace, Filename=fit_path)

        logger.information('Parameter file created : ' + par_path)
        logger.information('Fit file created : ' + fit_path)

    if (Plot != 'None'):
        ResNormPlot(fname,Plot)
    EndTime('ResNorm')

def ResNormAddSampleLogs(workspace, e_range, v_binning):
    energy_min, energy_max = e_range

    AddSampleLog(Workspace=workspace, LogName="energy_min", LogType="Number", LogText=str(energy_min))
    AddSampleLog(Workspace=workspace, LogName="energy_max", LogType="Number", LogText=str(energy_max))
    AddSampleLog(Workspace=workspace, LogName="van_binning", LogType="Number", LogText=str(v_binning))

def ResNormPlot(inputWS,Plot):
    if (Plot == 'Intensity' or Plot == 'All'):
        iWS = inputWS + '_ResNorm_Intensity'
        i_plot=mp.plotSpectrum(iWS,0,False)
    if (Plot == 'Stretch' or Plot == 'All'):
        sWS = inputWS + '_ResNorm_Stretch'
        s_plot=mp.plotSpectrum(sWS,0,False)
    if (Plot == 'Fit' or Plot == 'All'):
        fWS = inputWS + '_ResNorm_Fit'
        f_plot=mp.plotSpectrum(fWS,0,False)
