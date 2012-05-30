# QL main  - does both Res & Data options
#
from mantid.simpleapi import *
import mantidplot as mp
from mantid import config, logger, mtd
from IndirectCommon import *
import platform, math, os.path

if ( is_32bit_win() ):
	import erange_win32 as Er,   QLres_win32 as QLr
	import QLdata_win32 as QLd,  QLse_win32 as Qse
	import Quest_win32 as Que,   ResNorm_win32 as resnorm
	import CEfit_win32 as cefit, SSfit_win32 as ssfit
#elif ( is_64bit_rhel_6_2() ):
#	import erange_lnx64 as Er,   QLres_lnx64 as QLr
#	import QLdata_lnx64 as QLd,  QLse_lnx64 as Qse
#	import Quest_lnx64 as Que,   ResNorm_lnx64 as resnorm
#	import CEfit_lnx64 as cefit, SSfit_lnx64 as ssfit
else:
	sys.exit('F2Py Absorption Corrections programs NOT available on your operatingenvironment')

def PadArray(inarray,nfixed):
	npt=len(inarray)
	padding = nfixed-npt
	outarray=[]
	outarray.extend(inarray)
	outarray +=[0]*padding
	return outarray

def PadXY(Xin,Yin,array_len):
	X=PadArray(Xin,array_len)
	Y=PadArray(Yin,array_len)
	return X,Y

def PadXYE(Xin,Yin,Ein,array_len):
	X=PadArray(Xin,array_len)
	Y=PadArray(Yin,array_len)
	E=PadArray(Ein,array_len)
	return X,Y,E

def CalcErange(inWS,ns,er,nbin):
	rscl = 1.0
	array_len = 4096                           # length of array in Fortran
	N,X,Y,E = GetXYE(inWS,ns,array_len)
	nout,bnorm,Xdat=Er.erange(N,X,Y,E,er,nbin,rscl)
	return nout,bnorm,Xdat,X,Y,E

def GetXYE(inWS,n,array_len):
	Xin = mtd[inWS].readX(n)
	N = len(Xin)-1							# get no. points from length of x array
	Yin = mtd[inWS].readY(n)
	Ein = mtd[inWS].readE(n)
	X,Y,E = PadXYE(Xin,Yin,Ein,array_len)
	return N,X,Y,E

def GetResNorm(ngrp):
	if ngrp == 0:
		dtnorm = mtd['ResNorm_1'].readY(0)
		xscale = mtd['ResNorm_2'].readY(0)
	else:
		dtnorm = []
		xscale = []
		for m in range(0,ngrp):
			dtnorm.append(1.0)
			xscale.append(1.0)
	dtn,xsc = PadXY(dtnorm,xscale,51)
	return dtn,xsc
	
def QLStart(program,ana,samWS,resWS,rtype,rsname,erange,nbins,fitOp,wfile,Verbose=False,Plot=False):
	StartTime('QL '+rtype)
	if rtype == 'Res':
		rext = 'res'
	if rtype == 'Data':
		rext = 'red'
	workdir = config['defaultsave.directory']
	spath = os.path.join(workdir, samWS+'_'+ana+'_red.nxs')		# path name for sample nxs file
	LoadNexusProcessed(Filename=spath, OutputWorkspace=samWS)
	nsam = mtd[samWS].getNumberHistograms()                      # no. of hist/groups in sam
	rpath = os.path.join(workdir, resWS+'_'+ana+'_'+rext+'.nxs')		# path name for res nxs file
	LoadNexusProcessed(Filename=rpath, OutputWorkspace=resWS)
	if Verbose:
		logger.notice('Sample file is ' + spath)
		logger.notice('Resolution file is ' + rpath)
	if fitOp[3] == 1:
		path = os.path.join(workdir, rsname+'_ResNorm_Paras.nxs')	# path name for resnnrm nxs file
		LoadNexusProcessed(Filename=path, OutputWorkspace='ResNorm')
		Xin = mtd['ResNorm_1'].readX(0)
		nrm = len(Xin)						# no. points from length of x array
		if nrm != nsam:				# check that no. groups are the same
			error = 'ResNorm groups (' +str(nrm) + ') not = Sample (' +str(nsam) +')'			
			exit(error)
		else:
			if Verbose:
				logger.notice('ResNorm file is ' + path)
	array_len = 4096                           # length of array in Fortran
	efix = getEfixed(samWS)
	Xin = mtd[samWS].readX(0)
	ntc = len(Xin)-1						# no. points from length of x array
	ngrp = mtd[samWS].getNumberHistograms()       # no. of hist/groups in sample
	nres = mtd[resWS].getNumberHistograms()       # no. of hist/groups in res
	theta,Q = GetThetaQ(samWS)
	if program == 'QL':
		if nres == 1:
			prog = 'QLr'                        # res file
		else:
			prog = 'QLd'                        # data file
			if nres != ngrp:				# check that no. groups are the same
				error = 'Resolution histograms (' +str(nres) + ') not = Sample (' +str(ngrp) +')'			
				exit(error)
	if program == 'QSe':
		if nres == 1:
			prog = 'QSe'                        # res file
		else:
			error = 'Stretched Exp ONLY works with RES file'			
			exit(error)
	if Verbose:
		logger.notice('Version is ' +prog)
		logger.notice(' Number of spectra = '+str(ngrp))
	o_w1 = fitOp[2]
	Wy = []
	We = []
	if o_w1 == 1:
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
		if lasc != ngrp:				# check that no. groups are the same
			error = 'Width groups (' +str(nres) + ') not = Sample (' +str(ngrp) +')'	
			exit(error)
		else:
			for m in range(0,lasc):
				var = ExtractFloat(asc[m])
				Wy.append(var[0])
				We.append(var[1])
	else:
		for m in range(0,ngrp):
			Wy.append(0.0)
			We.append(0.0)
	Wy,We = PadXY(Wy,We,51)
	o_res = fitOp[3]
	if o_res == 0:
		dtn,xsc = GetResNorm(ngrp)
	if o_res == 1:
		dtn,xsc = GetResNorm(0)
	fname = samWS + '_'+ prog
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
	wrks=workdir + samWS
	if Verbose:
		logger.notice(' lptfile : ' + wrks +'_' +prog + '.lpt')
	lwrk=len(wrks)
	wrks.ljust(140,' ')
	wrkr=resWS
	wrkr.ljust(140,' ')
	wrk = [wrks, wrkr]
#
	if program == 'QL':
		xprob = []
		eprob = []
		prob0 = []
		prob1 = []
		prob2 = []
	nbin = nbins[0]
	nrbin = nbins[1]
	for m in range(0,ngrp):
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
		Nb,Xb,Yb,Eb = GetXYE(resWS,mm,array_len)
		numb = [ngrp, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin]
		rscl = 1.0
		reals = [efix, theta[m], rscl, bnorm]
		if prog == 'QLr':
			nd,xout,yout,eout,yfit,yprob=QLr.qlres(numb,Xv,Yv,Ev,reals,fitOp,
												   Xdat,Xb,Yb,Wy,We,dtn,xsc,
												   wrks,wrkr,lwrk)
			message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])+' '+str(yprob[2])+' '+str(yprob[3])
		if prog == 'QLd':
			nd,xout,yout,eout,yfit,yprob=QLd.qldata(numb,Xv,Yv,Ev,reals,fitOp,
												    Xdat,Xb,Yb,Eb,Wy,We,
													wrks,wrkr,lwrk)
			message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])+' '+str(yprob[2])+' '+str(yprob[3])
		if prog == 'QSe':
			nd,xout,yout,eout,yfit,yprob=Qse.qlstexp(numb,Xv,Yv,Ev,reals,fitOp,
													Xdat,Xb,Yb,Wy,We,dtn,xsc,
													wrks,wrkr,lwrk)
			message = ' Log(prob) : '+str(yprob[0])+' '+str(yprob[1])
		if Verbose:
			logger.notice(message)
		dataX = xout[:nd]
		dataX = numpy.append(dataX,2*xout[nd-1]-xout[nd-2])
		yfit_list = numpy.split(yfit[:4*nd],4)
		dataF0 = yfit_list[0]
		dataF1 = yfit_list[1]
		if program == 'QL':
			dataF2 = yfit_list[2]
			dataF3 = yfit_list[3]
		dataG = numpy.zeros(nd)
		if m == 0:
			CreateWorkspace(OutputWorkspace=fname+'_Data', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],
				Nspec=1, UnitX='DeltaE')
			CreateWorkspace(OutputWorkspace=fname+'_Fit1', DataX=dataX, DataY=dataF1[:nd], DataE=dataG,
				Nspec=1, UnitX='DeltaE')
			if program == 'QL':
				CreateWorkspace(OutputWorkspace=fname+'_Fit2', DataX=dataX, DataY=dataF2[:nd], DataE=dataG,
					Nspec=1, UnitX='DeltaE')
		else:
			CreateWorkspace(OutputWorkspace='__datmp', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],
				Nspec=1, UnitX='DeltaE')
			ConjoinWorkspaces(InputWorkspace1=fname+'_Data', InputWorkspace2='__datmp',CheckOverlapping=False)				
			CreateWorkspace(OutputWorkspace='__f1tmp', DataX=dataX, DataY=dataF1[:nd], DataE=dataG,
				Nspec=1, UnitX='DeltaE')
			ConjoinWorkspaces(InputWorkspace1=fname+'_Fit1', InputWorkspace2='__f1tmp',CheckOverlapping=False)				
			if program == 'QL':
				CreateWorkspace(OutputWorkspace='__f2tmp', DataX=dataX, DataY=dataF2[:nd], DataE=dataG,
					Nspec=1, UnitX='DeltaE')
				ConjoinWorkspaces(InputWorkspace1=fname+'_Fit2', InputWorkspace2='__f2tmp',CheckOverlapping=False)				
		Minus(LHSWorkspace=fname+'_Fit1', RHSWorkspace=fname+'_Data', OutputWorkspace=fname+'_Res1')
		if program == 'QL':
			Minus(LHSWorkspace=fname+'_Fit2', RHSWorkspace=fname+'_Data', OutputWorkspace=fname+'_Res2')
			prob0.append(yprob[0])
			prob1.append(yprob[1])
			prob2.append(yprob[2])
	if program == 'QL':
		group = fname+'_Data,'+fname+'_Fit1,'+fname+'_Res1,'+fname+'_Fit2,'+fname+'_Res2'
		GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS)
		xprob = []
		eprob = []
		for m in range(0,ngrp):
			xprob.append(Q[m])
			eprob.append(0.0)
		xPr = 3*xprob
		ePr = 3*eprob
		yPr = []
		for m in range(0,len(prob0)):
			yPr.append(prob0[m])
		for m in range(0,len(prob1)):
			yPr.append(prob1[m])
		for m in range(0,len(prob2)):
			yPr.append(prob2[m])
		CreateWorkspace(OutputWorkspace=probWS, DataX=xPr, DataY=yPr, DataE=ePr,
			Nspec=3, UnitX='MomentumTransfer')
	if program == 'QSe':
		group = fname+'_Data,'+fname+'_Fit1,'+fname+'_Res1'
		GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fitWS)
	fit_path = os.path.join(workdir,fitWS+'.nxs')
	SaveNexusProcessed(InputWorkspace=fitWS, Filename=fit_path)
	if Verbose:
		logger.notice('Output file created : ' + fit_path)
	if program == 'QL':
		C2Fw(prog,samWS)
		if (Plot != 'None'):
			QLPlotQL(fname,Plot)
	if program == 'QSe':
		C2Se(samWS)
		if (Plot != 'None'):
			QLPlotQSe(fname,Plot)
	EndTime('QL '+rtype)

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
	fname = sname +'_'+ prog
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
	fname = sname + '_'+ prog
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
	fname = sname +'_' + prog
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

def QuestStart(ana,samWS,resWS,nbs,erange,nbins,fitOp,Verbose=False,Plot=False):
	StartTime('Quest')
	workdir = config['defaultsave.directory']
	spath = os.path.join(workdir, samWS+'_'+ana+'_red.nxs')		# path name for sample nxs file
	LoadNexusProcessed(Filename=spath, OutputWorkspace=samWS)
	nsam = mtd[samWS].getNumberHistograms()                      # no. of hist/groups in sam
	rpath = os.path.join(workdir, resWS+'_'+ana+'_res.nxs')		# path name for res nxs file
	LoadNexusProcessed(Filename=rpath, OutputWorkspace=resWS)
	if Verbose:
		logger.notice('Sample file is ' + spath)
		logger.notice('Resolution file is ' + rpath)
		logger.notice(' Number of spectra = '+str(nsam))
	if fitOp[3] == 1:
		path = os.path.join(workdir, rsname+'_ResNorm_Paras.nxs')	# path name for resnnrm nxs file
		LoadNexusProcessed(Filename=path, OutputWorkspace='ResNorm')
		Xin = mtd['ResNorm_1'].readX(0)
		nrm = len(Xin)						# no. points from length of x array
		if nrm != nsam:				# check that no. groups are the same
			error = 'ResNorm groups (' +str(nrm) + ') not = Sample (' +str(nsam) +')'			
			exit(error)
		else:
			if Verbose:
				logger.notice('ResNorm file is ' + path)
	array_len = 4096                           # length of array in Fortran
	workdir = config['defaultsave.directory']
	efix = getEfixed(samWS)
	Xin = mtd[samWS].readX(0)
	ntc = len(Xin)-1						# no. points from length of x array
	ngrp = mtd[samWS].getNumberHistograms()       # no. of hist/groups in sample
	nres = mtd[resWS].getNumberHistograms()       # no. of hist/groups in res
	if nres == 1:
		prog = 'Qst'                        # res file
	else:
		error = 'Stretched Exp ONLY works with RES file'			
		exit(error)
	o_res = fitOp[3]
	if o_res == 0:
		dtn,xsc = GetResNorm(ngrp)
	if o_res == 1:
		dtn,xsc = GetResNorm(0)
	fname = samWS + '_'+ prog
	wrks=workdir + samWS
	if Verbose:
		logger.notice(' lptfile : ' + wrks +'_Qst.lpt')
	lwrk=len(wrks)
	wrks.ljust(140,' ')
	wrkr=resWS
	wrkr.ljust(140,' ')
	wrk = [wrks, wrkr]
#
	nbin = nbins[0]
	nrbin = nbins[1]
	theta,Q = GetThetaQ(samWS)
	Nbet = nbs[0]
	eBet0 = numpy.zeros(Nbet)
	Nsig = nbs[1]
	eSig0 = numpy.zeros(Nsig)
	rscl = 1.0
	Qaxis = ''
	for m in range(0,ngrp):
		if Verbose:
			logger.notice('Group ' +str(m)+ ' at angle '+ str(theta[m]))
		nsp = m+1
		nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(samWS,m,erange,nbin)
		Ndat = nout[0]
		Imin = nout[1]
		Imax = nout[2]
		Nb,Xb,Yb,Eb = GetXYE(resWS,0,array_len)
		numb = [ngrp, nsp, ntc, Ndat, nbin, Imin, Imax, Nb, nrbin, Nbet, Nsig]
		reals = [efix, theta[m], rscl, bnorm]
		xsout,ysout,xbout,ybout,zpout=Que.quest(numb,Xv,Yv,Ev,reals,fitOp,
											Xdat,Xb,Yb,wrks,wrkr,lwrk)
		dataXs = xsout[:Nsig]
		dataYs = ysout[:Nsig]
		dataXb = xbout[:Nbet]
		dataYb = ybout[:Nbet]
		zpWS = fname + '_Zp' +str(m)
		if (m > 0):
			Qaxis += ','
		Qaxis += str(Q[m])
		for n in range(0,Nsig):
			yfit_list = numpy.split(zpout[:Nsig*Nbet],Nsig)
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
			xSig = numpy.append(xSig,dataXs)
			ySig = numpy.append(ySig,dataYs)
			eSig = numpy.append(eSig,eSig0)
			xBet = numpy.append(xBet,dataXb)
			yBet = numpy.append(yBet,dataYb)
			eBet = numpy.append(eBet,eBet0)
			groupZ = groupZ +','+ zpWS
	CreateWorkspace(OutputWorkspace=fname+'_Sigma', DataX=xSig, DataY=ySig, DataE=eSig,
		Nspec=ngrp, UnitX='', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
	CreateWorkspace(OutputWorkspace=fname+'_Beta', DataX=xBet, DataY=yBet, DataE=eBet,
		Nspec=ngrp, UnitX='', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
	group = fname + '_Sigma,'+ fname + '_Beta'
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname+'_Fit')
	fpath = os.path.join(workdir,fname+'_Fit.nxs')
	SaveNexusProcessed(InputWorkspace=fname+'_Fit', Filename=fpath)
	GroupWorkspaces(InputWorkspaces=groupZ,OutputWorkspace=fname+'_Contour')
	cpath = os.path.join(workdir,fname+'_Contour.nxs')
	SaveNexusProcessed(InputWorkspace=fname+'_Contour', Filename=cpath)
	if Verbose:
		logger.notice('Output file for Fit : ' + fpath)
		logger.notice('Output file for Contours : ' + cpath)
	if (Plot != 'None'):
		QuestPlot(fname,Plot)
	EndTime('Quest')

def ResNormStart(ana,vanWS,resWS,erange,nbin,Verbose=False,Plot='None'):
	StartTime('ResNorm')
	workdir = config['defaultsave.directory']
	van = vanWS + '_' + ana
	vname = van + '_red'
	vpath = os.path.join(workdir, vname+'.nxs')		           # path name for van nxs file
	LoadNexusProcessed(Filename=vpath, OutputWorkspace=vname)
	rname = resWS + '_' + ana + '_res'
	rpath = os.path.join(workdir, rname+'.nxs')                # path name for res nxs file
	LoadNexusProcessed(Filename=rpath, OutputWorkspace=rname)
	if Verbose:
		logger.notice('Vanadium file is ' + vpath)
		logger.notice('Resolution file is ' + rpath)
	nvan = mtd[vname].getNumberHistograms()                      # no. of hist/groups in van
	nres = mtd[rname].getNumberHistograms()                      # no. of hist/groups in res
	array_len = 4096
	theta,Q = GetThetaQ(vname)
	efix = getEfixed(vname)
	nout,bnorm,Xdat,Xv,Yv,Ev = CalcErange(vname,0,erange,nbin)
	Ndat = nout[0]
	Imin = nout[1]
	Imax = nout[2]
	wrks=workdir + vanWS
	if Verbose:
		logger.notice(' Number of spectra = '+str(nvan))
		logger.notice(' lptfile : ' + wrks +'_resnrm.lpt')
	lwrk=len(wrks)
	wrks.ljust(140,' ')
	wrkr=rname
	wrkr.ljust(140,' ')
	Xin = mtd[rname].readX(0)
	Nb = len(Xin)-1											# get no. points from length of x array
	Yin = mtd[rname].readY(0)
	Ein = mtd[rname].readE(0)
	Xb,Yb = PadXY(Xin,Yin,array_len)
	parX = []
	par1 = []
	par2 = []
	parE = []
	rscl = 1.0
	theta,Q = GetThetaQ(vname)
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
		parX.append(theta[m])
		par1.append(pfit[0])
		par2.append(pfit[1])
		parE.append(0.0)
		dataX = xout[:nd]
		dataX = numpy.append(dataX,2*xout[nd-1]-xout[nd-2])
		if m == 0:
			CreateWorkspace(OutputWorkspace='Data', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],
				NSpec=1, UnitX='DeltaE')
			CreateWorkspace(OutputWorkspace='Fit', DataX=dataX, DataY=yfit[:nd], DataE=numpy.zeros(nd),
				NSpec=1, UnitX='DeltaE')
		else:
			CreateWorkspace(OutputWorkspace='__datmp', DataX=dataX, DataY=yout[:nd], DataE=eout[:nd],
				NSpec=1, UnitX='DeltaE')
			ConjoinWorkspaces(InputWorkspace1='Data', InputWorkspace2='__datmp', CheckOverlapping=False)				
			CreateWorkspace(OutputWorkspace='__f1tmp', DataX=dataX, DataY=yfit[:nd], DataE=numpy.zeros(nd),
				NSpec=1, UnitX='DeltaE')
			ConjoinWorkspaces(InputWorkspace1='Fit', InputWorkspace2='__f1tmp', CheckOverlapping=False)				
	CreateWorkspace(OutputWorkspace=van+'_ResNorm_Intensity', DataX=parX, DataY=par1, DataE=parE,
		NSpec=1, UnitX='MomentumTransfer')
	CreateWorkspace(OutputWorkspace=van+'_ResNorm_Stretch', DataX=parX, DataY=par2, DataE=parE,
		NSpec=1, UnitX='MomentumTransfer')
	group = van + '_ResNorm_Intensity,'+ van + '_ResNorm_Stretch'
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=van+'_ResNorm_Paras')
	par_path = os.path.join(workdir,van+'_ResNorm_Paras.nxs')
	SaveNexusProcessed(InputWorkspace=van+'_ResNorm_Paras', Filename=par_path)
	GroupWorkspaces(InputWorkspaces='Data,Fit',OutputWorkspace=van+'_ResNorm_Fit')
	fit_path = os.path.join(workdir,van+'_ResNorm_Fit.nxs')
	SaveNexusProcessed(InputWorkspace=van+'_ResNorm_Fit', Filename=fit_path)
	if Verbose:
		logger.notice('Output file created : ' + par_path)
		logger.notice('Output file created : ' + fit_path)
	if (Plot != 'None'):
		ResNormPlot(van,Plot)
	EndTime('ResNorm')

def JumpStart(sname,jump,prog,fw,Verbose=False,Plot=False):
	StartTime('Jump fit : '+jump+' from '+prog+' ; ')
	workdir = config['defaultsave.directory']
	file = sname+'_'+prog+'_Parameters'
	path = os.path.join(workdir, file+'.nxs')					# path name for nxs file
	LoadNexusProcessed(Filename=path, OutputWorkspace=file)
	if Verbose:
		logger.notice('Parameter file is ' + path)
	if fw == 'FW11':
		fwn = '1'
	if fw == 'FW21':
		fwn = '2'
	if fw == 'FW22':
		fwn = '3'
	samWS = file +'_'+ fwn
	Xin = mtd[samWS].readX(0)
	nd = len(Xin)-1											# get no. points from length of x array
	Yin = mtd[samWS].readY(0)
	Ein = mtd[samWS].readE(0)
	X,Y,E=PadXYE(Xin,Yin,Ein,1000)
	wrk = workdir + sname +'_'+ jump +'_'+ fw
	lwrk = len(wrk)
	wrk.ljust(120,' ')
	if jump == 'CE':
		kill,res,nout,Xout,Yout=cefit.cefit(nd,X,Y,E,wrk,lwrk)
		if Verbose:
			logger.notice(' Normalised Chi-squared = ' +str(res[0]))
			logger.notice(' Log10[Prob(Chudley-Elliot|{Data})] = ' +str(res[1]))
			logger.notice(' Coeff.  A =  ' +str(res[2])+ ' +- ' +str(res[3]))
			logger.notice(' Coeff.  K =  ' +str(res[4])+ ' +- ' +str(res[5]))
	if jump == 'SS':
		kill,res,nout,Xout,Yout=ssfit.ssfit(nd,X,Y,E,wrk,lwrk)
		if Verbose:
			logger.notice(' Normalised Chi-squared = ' +str(res[0]))
			logger.notice(' Log10[Prob(Singwi-Sjolander|{Data})] = ' +str(res[1]))
			logger.notice(' Coeff.  A =  ' +str(res[2])+ ' +- ' +str(res[3]))
			logger.notice(' Coeff.  RR =  ' +str(res[4])+ ' +- ' +str(res[5]))
	ftWS = sname +'_'+ jump + 'fit_' +fw
	CreateWorkspace(OutputWorkspace=ftWS+'_Fit', DataX=Xout[:nout], DataY=Yout[:nout], DataE=numpy.zeros(nout),
		Nspec=1, UnitX='MomentumTransfer')
	CloneWorkspace(InputWorkspace=samWS, OutputWorkspace=ftWS+'_Data')
	group = ftWS + '_Data,'+ ftWS +'_Fit'
	GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=ftWS)
	fit_path = os.path.join(workdir,ftWS+'.nxs')
	SaveNexusProcessed(InputWorkspace=ftWS, Filename=fit_path)
	if Verbose:
		logger.notice('Output file is ' + fit_path)
	if Plot:
		JumpPlot(ftWS)
	EndTime('Jump fit : '+jump+' from '+prog+' ; ')

def JumpPlot(inputWS):
    j_plot=mp.plotSpectrum(inputWS+'_Data',0,True)
    mp.mergePlots(j_plot,mp.plotSpectrum(inputWS+'_Fit',0,False))

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

def QLPlotQL(inputWS,Plot):
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
		fWS = [inputWS+'_Data', inputWS+'_Fit1', inputWS+'_Res1']
		f_plot=mp.plotSpectrum(fWS,0,False)

def QLPlotQSe(inputWS,Plot):
	if (Plot == 'Intensity' or Plot == 'All'):
		i_plot=mp.plotSpectrum(inputWS+'_Inty',0,True)
	if (Plot == 'FwHm' or Plot == 'All'):
		w_plot=mp.plotSpectrum(inputWS+'_FwHm',0,True)
	if (Plot == 'Beta' or Plot == 'All'):
		s_plot=mp.plotSpectrum(inputWS+'_Beta',0,True)
	if (Plot == 'Fit' or Plot == 'All'):
		fWS = [inputWS+'_Data', inputWS+'_Fit1', inputWS+'_Res1']
		f_plot=mp.plotSpectrum(fWS,0,False)

def QuestPlot(inputWS,Plot):
	if (Plot == 'Sigma' or Plot == 'All'):
		s_graph = mp.importMatrixWorkspace(inputWS+'_Sigma').plotGraph2D()
        s_layer = s_graph.activeLayer().setAxisTitle(2, 'Sigma')
	if (Plot == 'Beta' or Plot == 'All'):
		b_graph = mp.importMatrixWorkspace(inputWS+'_Beta').plotGraph2D()
        b_layer = b_graph.activeLayer().setAxisTitle(2, 'Beta')
