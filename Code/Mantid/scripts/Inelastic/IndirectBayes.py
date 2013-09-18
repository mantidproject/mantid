# Bayes routines 
# Fortran programs use fixed length arrays whereas Python has variable lenght lists
# Input : the Python list is padded to Fortrans length using procedure PadArray
# Output : the Fortran numpy array is sliced to Python length using dataY = yout[:ny]
#
from IndirectImport import *
if is_supported_f2py_platform():
    Er      = import_f2py("erange")
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

def CalcErange(inWS,ns,er,nbin):
	rscl = 1.0
	array_len = 4096                           # length of array in Fortran
	N,X,Y,E = GetXYE(inWS,ns,array_len)        # get data
	nout,bnorm,Xdat=Er.erange(N,X,Y,E,er,nbin,rscl)    # calculate energy range
	if nout[0] == 0:
		error = 'Erange - calculated points is Zero'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	if nout[1] == 0:
		error = 'Erange - calculated Imin is Zero'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	if nout[2] == 0:
		error = 'Erange - calculated Imax is Zero'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	return nout,bnorm,Xdat,X,Y,E

def GetXYE(inWS,n,array_len):
	Xin = mtd[inWS].readX(n)
	N = len(Xin)-1							# get no. points from length of x array
	Yin = mtd[inWS].readY(n)
	Ein = mtd[inWS].readE(n)
	X=PadArray(Xin,array_len)
	Y=PadArray(Yin,array_len)
	E=PadArray(Ein,array_len)
	return N,X,Y,E

def GetResNorm(ngrp):
	if ngrp == 0:                                # read values from WS
		dtnorm = mtd['ResNorm_1'].readY(0)
		xscale = mtd['ResNorm_2'].readY(0)
	else:                                        # constant values
		dtnorm = []
		xscale = []
		for m in range(0,ngrp):
			dtnorm.append(1.0)
			xscale.append(1.0)
	dtn=PadArray(dtnorm,51)                      # pad for Fortran call
	xsc=PadArray(xscale,51)
	return dtn,xsc
	
def ReadNormFile(o_res,nsam,Verbose):            # get norm & scale values
	if o_res == 1:                     # use ResNorm file option=o_res
		Xin = mtd['ResNorm_1'].readX(0)
		nrm = len(Xin)						# no. points from length of x array
		if nrm == 0:				
			error = 'ResNorm file has no dtnorm points'			
			logger.notice('ERROR *** ' + error)
			sys.exit(error)
		Xin = mtd['ResNorm_2'].readX(0)					# no. points from length of x array
		if len(Xin) == 0:				
			error = 'ResNorm file has no xscale points'			
			logger.notice('ERROR *** ' + error)
			sys.exit(error)
		if nrm != nsam:				# check that no. groups are the same
			error = 'ResNorm groups (' +str(nrm) + ') not = Sample (' +str(nsam) +')'			
			logger.notice('ERROR *** ' + error)
			sys.exit(error)
		else:
			dtn,xsc = GetResNorm(0)
	if o_res == 0:                     # do not use ResNorm file option=o_res
		dtn,xsc = GetResNorm(nsam)
	return dtn,xsc

def ReadWidthFile(op_w1,wfile,ngrp,Verbose):                       # reads width file ASCII
	workdir = config['defaultsave.directory']
	if op_w1 == 1:                               # use width1 data  option=o_w1
		if Verbose:
			w_path = os.path.join(workdir, wfile)					# path name for nxs file
			logger.notice('Width file is ' + w_path)
		handle = open(w_path, 'r')
		asc = []
		for line in handle:
			line = line.rstrip()
			asc.append(line)
		handle.close()
		lasc = len(asc)
		if lasc == 0:
			error = 'No groups in width file'	
			logger.notice('ERROR *** ' + error)
			sys.exit(error)
		if lasc != ngrp:				# check that no. groups are the same
			error = 'Width groups (' +str(lasc) + ') not = Sample (' +str(ngrp) +')'	
			logger.notice('ERROR *** ' + error)
			sys.exit(error)
	else:                                           # constant values
		Wy = []
		We = []
		for m in range(0,ngrp):
			Wy.append(0.0)
			We.append(0.0)
	Wy=PadArray(Wy,51)                             # pad for Fortran call
	We=PadArray(We,51)
	return Wy,We

def CheckBinning(nbins):
	nbin = nbins[0]
	if nbin == '0':
		error = 'Sample binning is Zero'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	if len(nbins) == 2:
		nrbin = nbins[1]
		if nrbin == '0':
			error = 'Resolution binning is Zero'			
			logger.notice('ERROR *** ' + error)
			sys.exit(error)
	else:
		nrbin = 1
	return nbin,nrbin

# QLines programs
def QLRun(program,samWS,resWS,rsname,erange,nbins,fitOp,wfile,Loop,Verbose,Plot,Save):
	StartTime(program)
	workdir = config['defaultsave.directory']
	facility = config['default.facility']
	array_len = 4096						   # length of array in Fortran
	CheckXrange(erange,'Energy')
	nbin,nrbin = CheckBinning(nbins)
	if Verbose:
		logger.notice('Sample is ' + samWS)
		logger.notice('Resolution is ' + resWS)
	if facility == 'ISIS':
		CheckAnalysers(samWS,resWS,Verbose)
		efix = getEfixed(samWS)
		theta,Q = GetThetaQ(samWS)
	nsam,ntc = CheckHistZero(samWS)
	if Loop != True:
		nsam = 1
	nres,ntr = CheckHistZero(resWS)
	if program == 'QL':
		if nres == 1:
			prog = 'QLr'						# res file
		else:
			prog = 'QLd'						# data file
			CheckHistSame(samWS,'Sample',resWS,'Resolution')
	if program == 'QSe':
		if nres == 1:
			prog = 'QSe'						# res file
		else:
			error = 'Stretched Exp ONLY works with RES file'			
			logger.notice('ERROR *** ' + error)
			sys.exit(error)
	if Verbose:
		logger.notice('Version is ' +prog)
		logger.notice(' Number of spectra = '+str(nsam))
		logger.notice(' Erange : '+str(erange[0])+' to '+str(erange[1]))
	Wy,We = ReadWidthFile(fitOp[2],wfile,nsam,Verbose)
	dtn,xsc = ReadNormFile(fitOp[3],nsam,Verbose)
	fname = samWS[:-4] + '_'+ prog
	probWS = fname + '_Prob'
	fitWS = fname + '_Fit'
	datWS = fname + '_Data'
	if program == 'QSe':
		fwWS = fname + '_FwHm'
		itWS = fname + '_Inty'
		beWS = fname + '_Beta'
	if program == 'QL':
		fit1WS = fname + '_Fit1'
		fit2WS = fname + '_Fit2'
		res1WS = fname + '_Res1'
		res2WS = fname + '_Res2'
	wrks=workdir + samWS[:-4]
	if Verbose:
		logger.notice(' lptfile : '+wrks+'_'+prog+'.lpt')
	lwrk=len(wrks)
	wrks.ljust(140,' ')
	wrkr=resWS
	wrkr.ljust(140,' ')
	wrk = [wrks, wrkr]
#
	if program == 'QL':						   # initialise probability list
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
	for m in range(0,nsam):
		if Verbose:
			logger.notice('Group ' +str(m)+ ' at angle '+ str(theta[m]))
		nsp = m+1
		nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(samWS,m,erange,nbin)
		Ndat = nout[0]
		Imin = nout[1]
		Imax = nout[2]
		if prog == 'QLd':
			mm = m
		else:
			mm = 0
		Nb,Xb,Yb,Eb = GetXYE(resWS,mm,array_len)	 # get resolution data
		numb = [nsam, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin]
		rscl = 1.0
		reals = [efix, theta[m], rscl, bnorm]
		if prog == 'QLr':
			nd,xout,yout,eout,yfit,yprob=QLr.qlres(numb,Xv,Yv,Ev,reals,fitOp,
												   Xdat,Xb,Yb,Wy,We,dtn,xsc,
												   wrks,wrkr,lwrk)
			message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])+' '+str(yprob[2])+' '+str(yprob[3])
			if Verbose:
				logger.notice(message)
		if prog == 'QLd':
			nd,xout,yout,eout,yfit,yprob=QLd.qldata(numb,Xv,Yv,Ev,reals,fitOp,
													Xdat,Xb,Yb,Eb,Wy,We,
													wrks,wrkr,lwrk)
			message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])+' '+str(yprob[2])+' '+str(yprob[3])
			if Verbose:
				logger.notice(message)
		if prog == 'QSe':
			nd,xout,yout,eout,yfit,yprob=Qse.qlstexp(numb,Xv,Yv,Ev,reals,fitOp,
													Xdat,Xb,Yb,Wy,We,dtn,xsc,
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
		fitWS = fname+'_Result'
		if nsam > 1:
			fout = fitWS +'_'+ str(m)
		else:
			fout = fitWS
		CreateWorkspace(OutputWorkspace=fout, DataX=datX, DataY=datY, DataE=datE,
			Nspec=nsp, UnitX='DeltaE', VerticalAxisUnit='Text', VerticalAxisValues=names)
		if m == 0:
			group = fout
		else:
			group += ',' + fout
	if nsam > 1:
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
		CreateWorkspace(OutputWorkspace=probWS, DataX=xProb, DataY=yProb, DataE=eProb,
			Nspec=3, UnitX='MomentumTransfer')
		C2Fw(samWS[:-4],fname)
		if (Plot != 'None'):
			QLPlotQL(fname,Plot,res_plot,Loop)
	if program == 'QSe':
		C2Se(fname)
		if (Plot != 'None'):
			QLPlotQSe(fname,Plot,res_plot,Loop)
	if Save:
		fit_path = os.path.join(workdir,fitWS+'.nxs')
		SaveNexusProcessed(InputWorkspace=fitWS, Filename=fit_path)
		if Verbose:
			logger.notice('Output file created : ' + fit_path)
	EndTime(program)


def LorBlock(a,first,nl):                                 #read Ascii block of Integers
	line1 = a[first]
	first += 1
	val = ExtractFloat(a[first])               #Q,AMAX,HWHM,BSCL,GSCL
	Q = val[0]
	AMAX = val[1]
	HWHM = val[2]
	BSCL = val[3]
	GSCL = val[4]
	first += 1
	val = ExtractFloat(a[first])               #A0,A1,A2,A4
	int0 = [AMAX*val[0]]
	bgd1 = BSCL*val[1]
	bgd2 = BSCL*val[2]
	zero = GSCL*val[3]
	first += 1
	val = ExtractFloat(a[first])                #AI,FWHM first peak
	fw = [2.*HWHM*val[1]]
	int = [AMAX*val[0]]
	if nl >= 2:
		first += 1
		val = ExtractFloat(a[first])            #AI,FWHM second peak
		fw.append(2.*HWHM*val[1])
		int.append(AMAX*val[0])
	if nl == 3:
		first += 1
		val = ExtractFloat(a[first])            #AI,FWHM third peak
		fw.append(2.*HWHM*val[1])
		int.append(AMAX*val[0])
	first += 1
	val = ExtractFloat(a[first])                 #SIG0
	int0.append(val[0])
	first += 1
	val = ExtractFloat(a[first])                  #SIGIK
	int.append(AMAX*math.sqrt(math.fabs(val[0])+1.0e-20))
	first += 1
	val = ExtractFloat(a[first])                  #SIGFK
	fw.append(2.0*HWHM*math.sqrt(math.fabs(val[0])+1.0e-20))
	if nl >= 2:                                      # second peak
		first += 1
		val = ExtractFloat(a[first])                  #SIGIK
		int.append(AMAX*math.sqrt(math.fabs(val[0])+1.0e-20))
		first += 1
		val = ExtractFloat(a[first])                  #SIGFK
		fw.append(2.0*HWHM*math.sqrt(math.fabs(val[0])+1.0e-20))
	if nl == 3:                                       # third peak
		first += 1
		val = ExtractFloat(a[first])                  #SIGIK
		int.append(AMAX*math.sqrt(math.fabs(val[0])+1.0e-20))
		first += 1
		val = ExtractFloat(a[first])                  #SIGFK
		fw.append(2.0*HWHM*math.sqrt(math.fabs(val[0])+1.0e-20))
	first += 1
	return first,Q,int0,fw,int                                      #values as list
	
def ReadQlFile(prog,sname,nl):
	workdir = config['defaultsave.directory']
	fname = sname
	file = fname + '.ql' +str(nl)
	handle = open(os.path.join(workdir, file), 'r')
	asc = []
	for line in handle:
		line = line.rstrip()
		asc.append(line)
	handle.close()
	lasc = len(asc)
	var = asc[3].split()							#split line on spaces
	nspec = var[0]
	ndat = var[1]
	var = ExtractInt(asc[6])
	first = 7
	Xout = []
	Yf1 = []
	Ef1 = []
	Yf2 = []
	Ef2 = []
	Yf3 = []
	Ef3 = []
	Yi1 = []
	Ei1 = []
	Yi2 = []
	Ei2 = []
	Yi3 = []
	Ei3 = []
	ns = int(nspec)
	for m in range(0,ns):
		if nl == 1:
			first,Q,int0,fw,it = LorBlock(asc,first,1)
			Xout.append(Q)
			Yf1.append(fw[0])
			Ef1.append(fw[1])
			Yi1.append(it[0])
			Ei1.append(it[1])
		if nl == 2:
			first,Q,int0,fw,it = LorBlock(asc,first,2)
			Xout.append(Q)
			Yf1.append(fw[0])
			Ef1.append(fw[2])
			Yf2.append(fw[1])
			Ef2.append(fw[3])
			Yi1.append(it[0])
			Ei1.append(it[2])
			Yi2.append(it[1])
			Ei2.append(it[3])
		if nl == 3:
			first,Q,int0,fw,it = LorBlock(asc,first,3)
			Xout.append(Q)
			Yf1.append(fw[0])
			Ef1.append(fw[3])
			Yf2.append(fw[1])
			Ef2.append(fw[4])
			Yf3.append(fw[2])
			Ef3.append(fw[5])
			Yi1.append(it[0])
			Ei1.append(it[3])
			Yi2.append(it[1])
			Ei2.append(it[4])
			Yi3.append(it[2])
			Ei3.append(it[5])
	if nl == 1:
		CreateWorkspace(OutputWorkspace=fname+'_FW11', DataX=Xout, DataY=Yf1, DataE=Ef1,
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_IT11', DataX=Xout, DataY=Yi1, DataE=Ei1,
			Nspec=1, UnitX='MomentumTransfer')
	if nl == 2:
		CreateWorkspace(OutputWorkspace=fname+'_FW21', DataX=Xout, DataY=Yf1, DataE=Ef1,
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_FW22', DataX=Xout, DataY=Yf2, DataE=Ef2,
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_IT21', DataX=Xout, DataY=Yi1, DataE=Ei1,
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_IT22', DataX=Xout, DataY=Yi2, DataE=Ei2,
			Nspec=1, UnitX='MomentumTransfer')
	if nl == 3:
		CreateWorkspace(OutputWorkspace=fname+'_FW31', DataX=Xout, DataY=Yf1, DataE=Ef1,
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_FW32', DataX=Xout, DataY=Yf2, DataE=Ef2,
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_FW33', DataX=Xout, DataY=Yf3, DataE=Ef3,	
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_IT31', DataX=Xout, DataY=Yi1, DataE=Ei1,	
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_IT32', DataX=Xout, DataY=Yi2, DataE=Ei2,
			Nspec=1, UnitX='MomentumTransfer')
		CreateWorkspace(OutputWorkspace=fname+'_IT33', DataX=Xout, DataY=Yi3, DataE=Ei3,
			Nspec=1, UnitX='MomentumTransfer')

def C2Fw(prog,sname):
	workdir = config['defaultsave.directory']
	fname = sname
	ReadQlFile(prog,sname,1)
	ReadQlFile(prog,sname,2)
	ReadQlFile(prog,sname,3)
	group2 = fname + '_FW21,'+ fname + '_FW22'
	group3 = fname + '_FW31,'+ fname + '_FW32,'+ fname + '_FW33'
	group = fname + '_FW11,'+  group2 +','+  group3
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname+'_FwHm')
	group2 = fname + '_IT21,'+ fname + '_IT22'
	group3 = fname + '_IT31,'+ fname + '_IT32,'+ fname + '_IT33'
	group = fname + '_IT11,'+ group2 +','+ group3
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname+'_Inty')
	group = fname + '_FwHm,'+ fname + '_Inty'
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname+'_Parameters')
	opath = os.path.join(workdir,fname+'_Parameters.nxs')
	SaveNexusProcessed(InputWorkspace=fname+'_Parameters', Filename=opath)

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
	workdir = config['defaultsave.directory']
	prog = 'QSe'
	fname = sname
	file = fname + '.qse'
	handle = open(os.path.join(workdir, file), 'r')
	asc = []
	for line in handle:
		line = line.rstrip()
		asc.append(line)
	handle.close()
	lasc = len(asc)
	var = asc[3].split()							#split line on spaces
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
	for m in range(0,ns):
		first,Q,int0,fw,it,be = SeBlock(asc,first)
		Xout.append(Q)
		Yf.append(fw[0])
		Ef.append(fw[1])
		Yi.append(it[0])
		Ei.append(it[1])
		Yb.append(be[0])
		Eb.append(be[1])
	CreateWorkspace(OutputWorkspace=fname+'_FwHm', DataX=Xout, DataY=Yf, DataE=Ef,
		Nspec=1, UnitX='MomentumTransfer')
	CreateWorkspace(OutputWorkspace=fname+'_Inty', DataX=Xout, DataY=Yi, DataE=Ei,
		Nspec=1, UnitX='MomentumTransfer')
	CreateWorkspace(OutputWorkspace=fname+'_Beta', DataX=Xout, DataY=Yb, DataE=Eb,
		Nspec=1, UnitX='MomentumTransfer')
	group = fname + '_FwHm,'+ fname + '_Inty,'+ fname + '_Beta'
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname+'_Parameters')
	opath = os.path.join(workdir,fname+'_Parameters.nxs')
	SaveNexusProcessed(InputWorkspace=fname+'_Parameters', Filename=opath)

def QLPlotQL(inputWS,Plot,res_plot,Loop):
	if Loop:
		if (Plot == 'Prob' or Plot == 'All'):
			pWS = inputWS+'_Prob'
			p_plot=mp.plotSpectrum(pWS,[1,2],False)
		if (Plot == 'Intensity' or Plot == 'All'):
			iWS = [inputWS+'_IT11', inputWS+'_IT21', inputWS+'_IT22']
			i_plot=mp.plotSpectrum(iWS,0,True)
		if (Plot == 'FwHm' or Plot == 'All'):
			wWS = [inputWS+'_FW11', inputWS+'_FW21', inputWS+'_FW22']
			w_plot=mp.plotSpectrum(wWS,0,True)
	if (Plot == 'Fit' or Plot == 'All'):
		fWS = inputWS+'_Result_0'
		f_plot=mp.plotSpectrum(fWS,res_plot,False)

def QLPlotQSe(inputWS,Plot,res_plot,Loop):
	if Loop:
		if (Plot == 'Intensity' or Plot == 'All'):
			i_plot=mp.plotSpectrum(inputWS+'_Inty',0,True)
		if (Plot == 'FwHm' or Plot == 'All'):
			w_plot=mp.plotSpectrum(inputWS+'_FwHm',0,True)
		if (Plot == 'Beta' or Plot == 'All'):
			s_plot=mp.plotSpectrum(inputWS+'_Beta',0,True)
	if (Plot == 'Fit' or Plot == 'All'):
		fWS = inputWS+'_Result_0'
		f_plot=mp.plotSpectrum(fWS,res_plot,False)

# Quest programs

def CheckBetSig(nbs):
	Nsig = int(nbs[1])
	if Nsig == 0:
		error = 'Number of sigma points is Zero'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	if Nsig > 200:
		error = 'Max number of sigma points is 200'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	Nbet = int(nbs[0])
	if Nbet == 0:
		error = 'Number of beta points is Zero'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	if Nbet > 200:
		error = 'Max number of beta points is 200'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	return Nbet,Nsig

def QuestRun(samWS,resWS,nbs,erange,nbins,fitOp,Loop,Verbose,Plot,Save):
	StartTime('Quest')
	workdir = config['defaultsave.directory']
	array_len = 4096                           # length of array in Fortran
	CheckXrange(erange,'Energy')
	nbin,nrbin = CheckBinning(nbins)
	if Verbose:
		logger.notice('Sample is ' + samWS)
		logger.notice('Resolution is ' + resWS)
	CheckAnalysers(samWS,resWS,Verbose)
	nsam,ntc = CheckHistZero(samWS)
	if Loop != True:
		nsam = 1
	efix = getEfixed(samWS)
	theta,Q = GetThetaQ(samWS)
	nres,ntr = CheckHistZero(resWS)
	if nres == 1:
		prog = 'Qst'                        # res file
	else:
		error = 'Stretched Exp ONLY works with RES file'			
		logger.notice('ERROR *** ' + error)
		sys.exit(error)
	if Verbose:
		logger.notice(' Number of spectra = '+str(nsam))
		logger.notice(' Erange : '+str(erange[0])+' to '+str(erange[1]))
	dtn,xsc = ReadNormFile(fitOp[3],nsam,Verbose)
	fname = samWS[:-4] + '_'+ prog
	wrks=workdir + samWS[:-4]
	if Verbose:
		logger.notice(' lptfile : ' + wrks +'_Qst.lpt')
	lwrk=len(wrks)
	wrks.ljust(140,' ')
	wrkr=resWS
	wrkr.ljust(140,' ')
	wrk = [wrks, wrkr]
	Nbet,Nsig = CheckBetSig(nbs)
	eBet0 = np.zeros(Nbet)                  # set errors to zero
	eSig0 = np.zeros(Nsig)                  # set errors to zero
	rscl = 1.0
	Qaxis = ''
	for m in range(0,nsam):
		if Verbose:
			logger.notice('Group ' +str(m)+ ' at angle '+ str(theta[m]))
		nsp = m+1
		nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(samWS,m,erange,nbin)
		Ndat = nout[0]
		Imin = nout[1]
		Imax = nout[2]
		Nb,Xb,Yb,Eb = GetXYE(resWS,0,array_len)
		numb = [nsam, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin, Nbet, Nsig]
		reals = [efix, theta[m], rscl, bnorm]
		xsout,ysout,xbout,ybout,zpout=Que.quest(numb,Xv,Yv,Ev,reals,fitOp,
											Xdat,Xb,Yb,wrks,wrkr,lwrk)
		dataXs = xsout[:Nsig]               # reduce from fixed Fortran array
		dataYs = ysout[:Nsig]
		dataXb = xbout[:Nbet]
		dataYb = ybout[:Nbet]
		zpWS = fname + '_Zp' +str(m)
		if (m > 0):
			Qaxis += ','
		Qaxis += str(Q[m])
		for n in range(0,Nsig):
			yfit_list = np.split(zpout[:Nsig*Nbet],Nsig)
			dataYzp = yfit_list[n]
			if n == 0:
				CreateWorkspace(OutputWorkspace=zpWS, DataX=xbout[:Nbet], DataY=dataYzp[:Nbet], DataE=eBet0,
					Nspec=1, UnitX='MomentumTransfer')
			else:
				CreateWorkspace(OutputWorkspace='__Zpt', DataX=xbout[:Nbet], DataY=dataYzp[:Nbet], DataE=eBet0,
					Nspec=1, UnitX='MomentumTransfer')
				ConjoinWorkspaces(InputWorkspace1=zpWS, InputWorkspace2='__Zpt', CheckOverlapping=False)				
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
	CreateWorkspace(OutputWorkspace=fname+'_Sigma', DataX=xSig, DataY=ySig, DataE=eSig,
		Nspec=nsam, UnitX='', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
	CreateWorkspace(OutputWorkspace=fname+'_Beta', DataX=xBet, DataY=yBet, DataE=eBet,
		Nspec=nsam, UnitX='', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
	group = fname + '_Sigma,'+ fname + '_Beta'
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname+'_Fit')	
	if Loop:
		GroupWorkspaces(InputWorkspaces=groupZ,OutputWorkspace=fname+'_Contour')
	if Save:
		fpath = os.path.join(workdir,fname+'_Fit.nxs')
		SaveNexusProcessed(InputWorkspace=fname+'_Fit', Filename=fpath)
		cpath = os.path.join(workdir,fname+'_Contour.nxs')
		SaveNexusProcessed(InputWorkspace=fname+'_Contour', Filename=cpath)
		if Verbose:
			logger.notice('Output file for Fit : ' + fpath)
			logger.notice('Output file for Contours : ' + cpath)
	if (Plot != 'None'):
		QuestPlot(fname,Plot)
	EndTime('Quest')

def QuestPlot(inputWS,Plot):
	if (Plot == 'Sigma' or Plot == 'All'):
		s_graph = mp.importMatrixWorkspace(inputWS+'_Sigma').plotGraph2D()
        s_layer = s_graph.activeLayer().setAxisTitle(2, 'Sigma')
	if (Plot == 'Beta' or Plot == 'All'):
		b_graph = mp.importMatrixWorkspace(inputWS+'_Beta').plotGraph2D()
        b_layer = b_graph.activeLayer().setAxisTitle(2, 'Beta')

# ResNorm programs

def ResNormRun(vname,rname,erange,nbins,Verbose,Plot,Save):
	StartTime('ResNorm')
	workdir = config['defaultsave.directory']
	array_len = 4096                                    # length of Fortran array
	CheckXrange(erange,'Energy')
	nbin,nrbin = CheckBinning(nbins)
	CheckAnalysers(vname,rname,Verbose)
	nvan,ntc = CheckHistZero(vname)
	theta,Q = GetThetaQ(vname)
	efix = getEfixed(vname)
	nres,ntr = CheckHistZero(rname)
	nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(vname,0,erange,nbin)
	Ndat = nout[0]
	Imin = nout[1]
	Imax = nout[2]
	wrks=workdir + vname[:-4]
	if Verbose:
		logger.notice(' Number of spectra = '+str(nvan))
		logger.notice(' lptfile : ' + wrks +'_resnrm.lpt')
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
		if Verbose:
			logger.notice('Group ' +str(m)+ ' at angle '+ str(theta[m]))
		ntc,Xv,Yv,Ev = GetXYE(vname,m,array_len)
		nsp = m+1
		numb = [nvan, nsp, ntc, Ndat, nbin, Imin, Imax, Nb]
		reals = [efix, theta[0], rscl, bnorm]
		nd,xout,yout,eout,yfit,pfit=resnorm.resnorm(numb,Xv,Yv,Ev,reals,
									Xdat,Xb,Yb,wrks,wrkr,lwrk)
		if Verbose:
			message = ' Fit paras : '+str(pfit[0])+' '+str(pfit[1])
			logger.notice(message)
		dataX = xout[:nd]
		dataX = np.append(dataX,2*xout[nd-1]-xout[nd-2])
		if m == 0:
			yPar1 = np.array([pfit[0]])
			yPar2 = np.array([pfit[1]])
			CreateWorkspace(OutputWorkspace='Data', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],
				NSpec=1, UnitX='DeltaE')
			CreateWorkspace(OutputWorkspace='Fit', DataX=dataX, DataY=yfit[:nd], DataE=np.zeros(nd),
				NSpec=1, UnitX='DeltaE')
		else:
			yPar1 = np.append(yPar1,pfit[0])
			yPar2 = np.append(yPar2,pfit[1])
			CreateWorkspace(OutputWorkspace='__datmp', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],
				NSpec=1, UnitX='DeltaE')
			ConjoinWorkspaces(InputWorkspace1='Data', InputWorkspace2='__datmp', CheckOverlapping=False)				
			CreateWorkspace(OutputWorkspace='__f1tmp', DataX=dataX, DataY=yfit[:nd], DataE=np.zeros(nd),
				NSpec=1, UnitX='DeltaE')
			ConjoinWorkspaces(InputWorkspace1='Fit', InputWorkspace2='__f1tmp', CheckOverlapping=False)				
	CreateWorkspace(OutputWorkspace=fname+'_ResNorm_Intensity', DataX=xPar, DataY=yPar1, DataE=xPar,
		NSpec=1, UnitX='MomentumTransfer')
	CreateWorkspace(OutputWorkspace=fname+'_ResNorm_Stretch', DataX=xPar, DataY=yPar2, DataE=xPar,
		NSpec=1, UnitX='MomentumTransfer')
	group = fname + '_ResNorm_Intensity,'+ fname + '_ResNorm_Stretch'
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname+'_ResNorm_Paras')
	GroupWorkspaces(InputWorkspaces='Data,Fit',OutputWorkspace=fname+'_ResNorm_Fit')
	if Save:
		par_path = os.path.join(workdir,fname+'_ResNorm_Paras.nxs')
		SaveNexusProcessed(InputWorkspace=fname+'_ResNorm_Paras', Filename=par_path)
		fit_path = os.path.join(workdir,fname+'_ResNorm_Fit.nxs')
		SaveNexusProcessed(InputWorkspace=fname+'_ResNorm_Fit', Filename=fit_path)
		if Verbose:
			logger.notice('Parameter file created : ' + par_path)
			logger.notice('Fit file created : ' + fit_path)
	if (Plot != 'None'):
		ResNormPlot(fname,Plot)
	EndTime('ResNorm')

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

# Jump programs
def JumpRun(samWS,jump,Verbose,Plot,Save):
# Chudley-Elliott    HWHM=A*(1-sin*(Q*K)/(Q*K))
# for Q->0 W=A*Q^2*K^2/6
# Singwi-Sjolander  HWHM=A*(1-exp(-r*Q^2))
# for Q->0 W=A*Q^2*r
	StartTime('Jump fit : '+jump+' ; ')
	workdir = config['defaultsave.directory']
	if Verbose:
		logger.notice('Parameters in ' + samWS+'_Workspace')
	x = mtd[samWS].readX(0)
	xmax = x[len(x)-1]
	if jump == 'CE':
		aval = xmax
		bval = 1.5
		func = 'name=UserFunction, Formula=a*(1-(sin(x*b))/(x*b)), a='+str(aval)+', b='+str(bval)
	if jump == 'SS':
		aval = xmax
		bval = 0.24
		func = 'name=UserFunction, Formula=a*(1-exp(-x*x*b)), a='+str(aval)+', b='+str(bval)
	fitWS = samWS +'_'+jump +'fit'
	Fit(Function=func, InputWorkspace=samWS, CreateOutput=True, Output=fitWS)
	if Save:
		fit_path = os.path.join(workdir,fitWS+'.nxs')
		SaveNexusProcessed(InputWorkspace=fitWS, Filename=fit_path)
		if Verbose:
			logger.notice('Fit file is ' + fit_path)
	if Plot:
		JumpPlot(fitWS+'_Workspace')
	EndTime('Jump fit : '+jump+' ; ')

def JumpPlot(inputWS):
    j_plot=mp.plotSpectrum(inputWS+'_Data',0,True)
    mp.mergePlots(j_plot,mp.plotSpectrum(inputWS+'_Fit',0,False))
