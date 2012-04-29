#Force for ILL backscattering raw
from mantidsimple import *
import mantidplot as mp
import math, os.path as op
from IndirectCommon import *

#  Routines for Ascii file of raw data

def ExtractFloat(a):                              #extract values from line of ascii
	extracted = []
	elements = a.split()							#split line on spaces
	for n in elements:
		extracted.append(float(n))
	return extracted                                 #values as list

def ExtractInt(a):                              #extract values from line of ascii
	extracted = []
	elements = a.split()							#split line on spaces
	for n in elements:
		extracted.append(int(n))
	return extracted                                 #values as list

def Iblock(a,first):                                 #read Ascii block of Integers
	line1 = a[first]
	line2 = a[first+1]
	val = ExtractInt(line2)
	numb = val[0]
	lines=numb/10
	last = numb-10*lines
	if line1.startswith('I'):
		err = ''
	else:
		err = 'ERROR ** NOT an I block starting at line ' +str(first)
		exit(err)
	ival = []
	for m in range(0, lines):
		mm = first+2+m
		val = ExtractInt(a[mm])
		for n in range(0, 10):
			ival.append(val[n])
	mm += 1
	val = ExtractInt(a[mm])
	for n in range(0, last):
		ival.append(val[n])
	mm += 1
	return mm,ival                                       #values as list

def Fblock(a,first):                                 #read Ascii block of Floats 
	line1 = a[first]
	line2 = a[first+1]
	val = ExtractInt(line2)
	numb = val[0]
	lines=numb/5
	last = numb-5*lines
	if line1.startswith('F'):
		err= ''
	else:
		err = 'ERROR ** NOT an F block starting at line ' +str(first)
		exit(err)
	fval = []
	for m in range(0, lines): 
		mm = first+2+m
		val = ExtractFloat(a[mm])
		for n in range(0, 5):
			fval.append(val[n])
	mm += 1
	val = ExtractFloat(a[mm])
	for n in range(0, last):
		fval.append(val[n])
	mm += 1
	return mm,fval                                       #values as list

def ReadIbackGroup(a,first):                           #read Ascii block of spectrum values
	x = []
	y = []
	e = []
	next = first
	line1 = a[next]
	next += 1
	val = ExtractInt(a[next])
	n1 = val[0]
	ngrp = val[2]
	if line1.startswith('S'):
		err = ''
	else:
		err = 'ERROR ** NOT an S block starting at line ' +str(first)
		exit(err)
	next += 1
	next,Ival = Iblock(a,next)
	for m in range(0, len(Ival)): 
		x.append(float(m))
		yy = float(Ival[m])
		y.append(yy)
		ee = math.sqrt(yy)
		e.append(ee)
	return next,x,y,e                                #values of x,y,e as lists

def IbackStart(instr,run,Verbose=False,Plot=False):      #Ascii start routine
	StartTime('Iback')
	workdir = config['defaultsave.directory']
	file = instr +'_'+ run
	filext = file + '.asc'
	path = op.join(workdir, filext)
	if Verbose:
		logger.notice('Reading file : ' + path)
	handle = open(path, 'r')
	asc = []
	for line in handle:
		line = line.rstrip()
		asc.append(line)
	handle.close()
	lasc = len(asc)
# raw head
	text = asc[1]
	run = text[:8]
	first = 5
	next,Ival = Iblock(asc,first)
	next += 2
	title = asc[next]    # title line
	next += 1
	text = asc[next]   # user line
	user = text[20:32]
	time = text[40:50]
	next += 6           # 5 lines of text
# back head1
	next,Fval = Fblock(asc,next)
	if instr == 'IN10':
		freq = Fval[89]
	if instr == 'IN16':
		freq = Fval[2]
		amp = Fval[3]
		wave = Fval[69]
		Ef = 81.787/(4.0*wave*wave)
		npt = int(Fval[6])
		nsp = int(Fval[7])
# back head2
	next,Fval = Fblock(asc,next)
	k0 = 4.0*math.pi/wave
	d2r = math.pi/180.0
	theta = []
	Q = []
	for m in range(0, nsp): 
		theta.append(Fval[m])
# raw spectra
	val = ExtractInt(asc[next+3])
	npt = val[0]
	lgrp=5+npt/10
	val = ExtractInt(asc[next+1])
	if instr == 'IN10':
		nsp = int(val[2])
	if Verbose:
		logger.notice('Number of spectra : ' + str(nsp))
# read monitor
	nmon = next+nsp*lgrp
	nm,xm,ym,em = ReadIbackGroup(asc,nmon)
# monitor calcs
	imin = 0
	ymax = 0
	for m in range(0, 20):
		if ym[m] > ymax:
			imin=m
			ymax=ym[m]
	npt = len(ym)
	imax = npt
	ymax = 0
	for m in range(npt-1, npt-20, -1):
		if ym[m] > ymax:
			imax=m
			ymax=ym[m]
	new=imax-imin
	imid=new/2+1
	if instr == 'IN10':
		DRV=18.706							# fast drive
		vmax=freq*DRV
	if instr == 'IN16':
		vmax=1.2992581918414711e-4*freq*amp*2.0/wave 	#max energy
	dele=2.0*vmax/new
	xMon = []
	yOut = []
	eOut = []
	for m in range(0, new+1):
		xe = (m-imid)*dele
		mm = m+imin
		xMon.append(xe)
		yOut.append(ym[mm]/100.0)
		eOut.append(em[mm]/10.0)
	monWS = '__Mon'
	CreateWorkspace(OutputWorkspace=monWS, DataX=xMon, DataY=yOut, DataE=eOut,
		Nspec=1, UnitX='Energy')
#
	Qaxis = ''
	xDat = []
	yDat = []
	eDat = []
	for n in range(0, nsp):
		if Verbose:
			logger.notice('Spectrum ' + str(n+1) +' at angle '+ str(theta[n]))
		next,x,y,e = ReadIbackGroup(asc,next)
		for m in range(0, new+1):
			mm = m+imin
			xDat.append(xMon[m])
			yDat.append(y[mm])
			eDat.append(e[mm])
		if n != 0:
			Qaxis += ','
		Qaxis += str(theta[n])
	outWS = file
	CreateWorkspace(OutputWorkspace=outWS, DataX=xDat, DataY=yDat, DataE=eDat,
		Nspec=nsp, UnitX='Energy')
	Divide(LHSWorkspace=outWS, RHSWorkspace=monWS, OutputWorkspace=outWS)
	DeleteWorkspace(monWS)								# delete monitor WS
	opath = op.join(workdir,outWS+'_red.nxs')
	SaveNexusProcessed(InputWorkspace=outWS, Filename=opath)
	if Verbose:
		logger.notice('Output file : ' + opath)
	if (Plot != 'None'):
		plotForce(outWS,Plot)
	EndTime('Iback')

# Routines for Inx ascii file

def ReadInxGroup(asc,n,lgrp):                  # read ascii x,y,e
	x = []
	y = []
	e = []
	first = n*lgrp
	last = (n+1)*lgrp
	val = ExtractFloat(asc[first+2])
	Q = val[0]
	for m in range(first+4, last): 
		val = ExtractFloat(asc[m])
		xin = val[0]/1000.0
		x.append(xin)
		y.append(val[1])
		e.append(val[2])
	npt = len(x)
	return Q,npt,x,y,e                                 #values of x,y,e as lists

def InxStart(instr,run,Verbose=False,Plot=False):
	StartTime('Inx')
	workdir = config['defaultsave.directory']
	file = instr +'_'+ run
	filext = file + '.inx'
	path = op.join(workdir, filext)
	if Verbose:
		logger.notice('Reading file : ' + path)
	handle = open(path, 'r')
	asc = []
	for line in handle:
		line = line.rstrip()
		asc.append(line)
	handle.close()
	lasc = len(asc)
	val = ExtractInt(asc[0])
	lgrp = int(val[0])
	ngrp = int(val[2])
	npt = int(val[7])
	title = asc[1]
	ltot = ngrp*lgrp
	if Verbose:
		logger.notice('Number of spectra : ' + str(ngrp))
	if ltot == lasc:
		err = ''
	else:
		err = 'ERROR ** file ' +filext+ ' should be ' +str(ltot)+ ' lines'
		sys.exit(err)
	outWS = file
	Qaxis = ''
	xDat = []
	yDat = []
	eDat = []
	for m in range(0,ngrp):
		Qq,nd,xd,yd,ed = ReadInxGroup(asc,m,lgrp)
		if Verbose:
			logger.notice('Spectrum ' + str(m+1) +' at Q= '+ str(Qq))
		if m != 0:
			Qaxis += ','
		Qaxis += str(Qq)
		for n in range(0,nd):
			xDat.append(xd[n])
			yDat.append(yd[n])
			eDat.append(ed[n])
	CreateWorkspace(OutputWorkspace=outWS, DataX=xDat, DataY=yDat, DataE=eDat,
		Nspec=ngrp, UnitX='Energy', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
	opath = op.join(workdir,outWS+'_red.nxs')
	SaveNexusProcessed(InputWorkspace=outWS, Filename=opath)
	if Verbose:
		logger.notice('Output file : ' + opath)
	if (Plot != 'None'):
		plotForce(outWS,Plot)
	EndTime('Inx')

def plotForce(inWS,Plot):
	if (Plot == 'Spectrum' or Plot == 'Both'):
		nHist = mtd[inWS].getNumberHistograms()
		plot_list = []
		for i in range(0, nHist):
			plot_list.append(i)
		res_plot=mp.plotSpectrum(inWS,plot_list)
	if (Plot == 'Contour' or Plot == 'Both'):
		cont_plot=mp.importMatrixWorkspace(inWS).plotGraph2D()



