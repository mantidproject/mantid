from math import *
from mantidsimple import *
from mantidplot import *
#import qti as qti
import numpy as n

def addRuns(runlist,wname):
  mantid.deleteWorkspace(str(wname))
  output=str(wname)
  if runlist[0] != "0":
    #nzeros=8-len(str(runlist[0]))
    #fpad=""
    #for i in range(nzeros):
    #  fpad+="0"
    #filename="offspec"+fpad+str(runlist[0])+".nxs"
    #fname=str(FileFinder.findRuns(filename))
    #fname=str.replace(fname,'\\','/')
    #fname=str.replace(fname,'[','')
    #fname=str.replace(fname,']','')
    #fname=str.replace(fname,'\'','')
    #fname=fname.lower()
    ##fname=str.replace(fname,'.nxs','.raw')
    #Load(fname,output)
	Load(str(runlist[0]),output)
  else:
    #dae="ndx"+mtd.settings['default.instrument'].lower()
    dae="ndxoffspec"
    LoadDAE(DAEname=dae,OutputWorkspace=output,SpectrumMin="1")
    if(mtd[output].isGroup()):
    	for k in mtd[output].getNames():
    		mtd[k].setYUnit('Counts')
    else:
    	mtd[output].setYUnit('Counts')

  if len(runlist) > 1:
    for i in range(1,len(runlist)):
      if runlist[i] != "0":
        #nzeros=8-len(str(runlist[i]))
        #fpad=""
        #for j in range(nzeros):
        #  fpad+="0"
        #filename="offspec"+fpad+str(runlist[i])+".nxs"
        #fname=str(FileFinder.findRuns(filename))
        #fname=str.replace(fname,'\\','/')
        #fname=str.replace(fname,'[','')
        #fname=str.replace(fname,']','')
        #fname=str.replace(fname,'\'','')
        #fname=fname.lower()
        ##fname=str.replace(fname,'.nxs','.raw')
        #Load(fname,"wtemp")
		Load(str(runlist[0]),"wtemp")
      else:
		#dae="ndx"+mtd.settings['default.instrument'].lower()
		dae="ndxoffspec"
		LoadDAE(DAEname=dae,OutputWorkspace="wtemp",SpectrumMin="1")
		if(mtd['wtemp'].isGroup()):
			for k in mtd['wtemp'].getNames():
				mtd[k].setYUnit('Counts')
		else:
			mtd[output].setYUnit('Counts')
      Plus(output,"wtemp",output)
      mantid.deleteWorkspace("wtemp")

  mtd.sendLogMessage("addRuns Completed")
#
#===================================================================================================================
#
'''
parse a text string of the format "1-6:2+8+9,10+11+12+13-19:3,20-24"
to return a structure containing the separated lists [1, 3, 5, 8, 9], 
[10, 11, 12, 13, 16, 19] and [20, 21, 22, 23, 24]
as integer lists that addRuns can handle.
'''
def parseRunList(istring):
	if len(istring) >0: 
		s1=istring.split(',')
		rlist1=[]
		for i in range(len(s1)):
			tstr=s1[i].strip()
			if len(tstr) > 0:
				rlist1.append(tstr)
		rlist=[]
		for i in range(len(rlist1)):
			rlist2=[]
			if rlist1[i].find('+') >= 0:
				tstr=rlist1[i].split('+')
				for j in range(len(tstr)):
					if tstr[j].find(':') >=0 and tstr[j].find('-') >=0:
						tstr[j].strip()
						tstr2=tstr[j].split('-')
						tstr3=tstr2[1].split(':')
						r1=range(int(tstr2[0]),int(tstr3[0])+1,int(tstr3[1]))
						for k in r1:
							rlist2.append(str(k))
					elif tstr[j].find('-') >=0:
						tstr[j].strip()
						tstr2=tstr[j].split('-')
						r1=range(int(tstr2[0]),int(tstr2[1])+1)
						for k in r1:
							rlist2.append(str(k))
					else:
						rlist2.append(tstr[j])
			else:
				if rlist1[i].find(':') >=0 and rlist1[i].find('-')>=0:
					rlist1[i].strip()
					tstr2=rlist1[i].split('-')
					tstr3=tstr2[1].split(':')
					r1=range(int(tstr2[0]),int(tstr3[0])+1,int(tstr3[1]))
					for k in r1:
						rlist2.append(str(k))
				elif rlist1[i].find('-')>=0:
					rlist1[i].strip()
					tstr2=rlist1[i].split('-')
					r1=range(int(tstr2[0]),int(tstr2[1])+1)
					for k in r1:
						rlist2.append(str(k))
				else:
					rlist2.append(rlist1[i])
			rlist.append(rlist2)
	return rlist
#
#===================================================================================================================
#
def parseNameList(istring):
	s1=istring.split(',')
	namelist=[]
	for i in range(len(s1)):
		tstr=s1[i].strip()
		namelist.append(tstr)	
	return namelist

#
#===================================================================================================================
#
def floodnorm(wkspName,floodfile):
   #
   # pixel by pixel efficiency correction for the linear detector
   #
   floodloaded=0
   a1=mantid.getWorkspaceNames()
   for i in range(len(a1)):
       if a1[i] == "ld240flood":
           floodloaded=1
   if floodloaded == 0:
       LoadNexusProcessed(Filename=floodfile,OutputWorkspace="ld240flood")

   Divide(wkspName,"ld240flood",wkspName)
#
#===================================================================================================================
#
# Plot a bunch of workspaces as 2D maps
# using the supplied limits and log scale settings
#
def plot2D(wkspNames,limits,logScales):
	nplot=0
	workspace_mtx=[]
	wNames=parseNameList(wkspNames)
	for i in range(len(wNames)):
		w1=mtd[wNames[i]]
		if w1.isGroup():
			w1names=w1.getNames()
			for j in range(len(w1names)):
				#workspace_mtx.append(qti.app.mantidUI.importMatrixWorkspace(w1names[j]))
				workspace_mtx.append(mantidplot.importMatrixWorkspace(w1names[j]))
				gr2d=workspace_mtx[nplot].plotGraph2D()
				nplot=nplot+1
				l=gr2d.activeLayer()
				if logScales[0]=="0":
					l.setAxisScale(mantidplot.Layer.Bottom,float(limits[0]),float(limits[1]))
				elif logScales[0]=="2":
					l.setAxisScale(mantidplot.Layer.Bottom,float(limits[0]),float(limits[1]),1)
				if logScales[1]=="0":
					l.setAxisScale(mantidplot.Layer.Left,float(limits[2]),float(limits[3]))
				elif logScales[1]=="2":
					l.setAxisScale(mantidplot.Layer.Left,float(limits[2]),float(limits[3]),1)
				if logScales[2]=="0":
					l.setAxisScale(mantidplot.Layer.Right,float(limits[4]),float(limits[5]))
				elif logScales[2]=="2":
					l.setAxisScale(mantidplot.Layer.Right,float(limits[4]),float(limits[5]),1)
		else:
			workspace_mtx.append(mantidplot.importMatrixWorkspace(wNames[i]))
			gr2d=workspace_mtx[nplot].plotGraph2D()
			nplot=nplot+1
			l=gr2d.activeLayer()
			if logScales[0]=="0":
				l.setAxisScale(mantidplot.Layer.Bottom,float(limits[0]),float(limits[1]))
			elif logScales[0]=="2":
				l.setAxisScale(mantidplot.Layer.Bottom,float(limits[0]),float(limits[1]),1)
			if logScales[1]=="0":
				l.setAxisScale(mantidplot.Layer.Left,float(limits[2]),float(limits[3]))
			elif logScales[1]=="2":
				l.setAxisScale(mantidplot.Layer.Left,float(limits[2]),float(limits[3]),1)
			if logScales[2]=="0":
				l.setAxisScale(mantidplot.Layer.Right,float(limits[4]),float(limits[5]))
			elif logScales[2]=="2":
				l.setAxisScale(mantidplot.Layer.Right,float(limits[4]),float(limits[5]),1)
		
	mtd.sendLogMessage("plot2D finished")
#
#===================================================================================================================
#
# Plot a bunch of workspaces as 2D maps
# using the supplied limits and log scale settings
#
def XYPlot(wkspNames,spectra,limits,logScales,errors,singleFigure):
	wNames=parseNameList(wkspNames)
	spec=parseNameList(spectra)
	ploterr=0
	xLog=0
	yLog=0
	if errors == "2":
		ploterr=1
	if logScales[0] == "2":
		xLog=1
	if logScales[1] == "2":
		yLog=1
		
	if singleFigure == "2":
		p1=plotSpectrum(wNames,spec,ploterr)
		l=p1.activeLayer()
		l.setAxisScale(mantidplot.Layer.Bottom,float(limits[0]),float(limits[1]),xLog)
		l.setAxisScale(mantidplot.Layer.Left,float(limits[2]),float(limits[3]),yLog)
	else:	
		for i in range(len(wNames)):
			p1=plotSpectrum(wNames[i],spec,ploterr)
			l=p1.activeLayer()
			l.setAxisScale(mantidplot.Layer.Bottom,float(limits[0]),float(limits[1]),xLog)
			l.setAxisScale(mantidplot.Layer.Left,float(limits[2]),float(limits[3]),yLog)
		
	mtd.sendLogMessage("XYPlot finished")
#
#===================================================================================================================
#
def nrtestfn(runlist,wnames):
	
	rlist=parseRunList(runlist)
	mtd.sendLogMessage("This is the runlist:"+str(rlist))
	namelist=parseNameList(wnames)
	mtd.sendLogMessage("This is the nameslist:"+str(namelist))
	for i in range(len(rlist)):
		addRuns(rlist[i],namelist[i])
		#Load(Filename="L:/RawData/cycle_10_1/OFFSPEC0000"+str(rlist[i][j])+".nxs",OutputWorkspace="w"+str(rlist[i][j]),LoaderName="LoadISISNexus")

	mtd.sendLogMessage("nrtestfn completed")
'''
	output="w7503"
	plotper=[1,2]
	rebpars="0.5,0.025,14.5"
	spmin=0
	spmax=239
	Load(Filename="L:/RawData/cycle_10_1/OFFSPEC00007503.nxs",OutputWorkspace=output,LoaderName="LoadISISNexus")
	workspace_mtx=[]
	nplot=0
	for i in plotper:
		ConvertUnits(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i),Target="Wavelength",AlignBins="1")
		Rebin(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i),Params=rebpars)
		CropWorkspace(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i)+"m",StartWorkspaceIndex="1",EndWorkspaceIndex="1")
		CropWorkspace(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i)+"d",StartWorkspaceIndex=str(spmin+4),EndWorkspaceIndex=str(spmax+4))
		Divide(output+"_"+str(i)+"d",output+"_"+str(i)+"m",output+"_"+str(i)+"n")
		workspace_mtx.append(qti.app.mantidUI.importMatrixWorkspace(output+"_"+str(i)+"n"))
		gr2d=workspace_mtx[nplot].plotGraph2D()
		nplot=nplot+1
		l=gr2d.activeLayer()
	
	mtd.sendLogMessage("quickPlot Finished")
'''
def removeoutlayer(wksp):
	# remove counts from bins where there are so few counts it makes a mess of the polarisation
	# calculation
	a1=mantid.getMatrixWorkspace(wksp)
	nspec=a1.getNumberHistograms()
	x=a1.readX(0)
	for i in range(nspec):
		for j in range(len(x)-1):
			y=a1.readY(i)[j]
			if (y<2):
				a1.dataY(i)[j]=0.0;
				a1.dataE(i)[j]=0.0;

def nrSESANSFn(runList,nameList,P0runList,P0nameList,minSpec,maxSpec,upPeriod,downPeriod,existingP0,SEConstants,gparams,convertToSEL,lnPOverLam):
	nlist=parseNameList(nameList)
	mtd.sendLogMessage("This is the sample nameslist:"+str(nlist))
	rlist=parseRunList(runList)
	mtd.sendLogMessage("This is the sample runlist:"+str(rlist))
	for i in range(len(rlist)):
		addRuns(rlist[i],nlist[i])

	P0nlist=parseNameList(P0nameList)
	mtd.sendLogMessage("This is the P0nameslist:"+str(P0nlist))
	if existingP0 != "2":
		P0rlist=parseRunList(P0runList)
		mtd.sendLogMessage("This is the P0runlist:"+str(P0rlist))
		for i in range(len(P0rlist)):
			addRuns(P0rlist[i],P0nlist[i])

	mon_spec=int(gparams[3])-1
	minSp=int(minSpec)-1
	maxSp=int(maxSpec)-1
	reb=gparams[0]+","+gparams[1]+","+gparams[2]
	for i in nlist:
		a1=mantid.getMatrixWorkspace(i+"_1")
		nspec=a1.getNumberHistograms()
		ConvertUnits(i,i,"Wavelength",AlignBins=1)
		Rebin(i,i,reb)
		#removeoutlayer(i+"_1")
		#removeoutlayer(i+"_2")
		CropWorkspace(i,i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
		if nspec > 4:
			CropWorkspace(i,i+"2ddet",StartWorkspaceIndex=4,EndWorkspaceIndex=244)
		if int(maxSpec) > int(minSpec):
			SumSpectra(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)
		else:
			CropWorkspace(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)

		Divide(i+"det",i+"mon",i+"norm")
		if nspec > 4:
			Divide(i+"2ddet",i+"mon",i+"2dnorm")
		DeleteWorkspace(i+"mon")
		DeleteWorkspace(i+"det")
		DeleteWorkspace(i)
		Minus(i+"norm_"+upPeriod,i+"norm_"+downPeriod,"num")
		Plus(i+"norm_2",i+"norm_1","den")
		Divide("num","den",i+"pol")
		ReplaceSpecialValues(i+"pol",i+"pol",0.0,0.0,0.0,0.0)
		if nspec >4:
			#print i+"2dnorm_"+upPeriod
			#print i+"2dnorm_"+downPeriod
			Minus(i+"2dnorm_"+upPeriod,i+"2dnorm_"+downPeriod,"num")
			Plus(i+"2dnorm_2",i+"2dnorm_1","den")
			Divide("num","den",i+"2dpol")
			ReplaceSpecialValues(i+"2dpol",i+"2dpol",0.0,0.0,0.0,0.0)
		#DeleteWorkspace(i+"norm_2")
		#DeleteWorkspace(i+"norm_1")
		DeleteWorkspace("num")
		DeleteWorkspace("den")
		
	if existingP0 != "2":
		for i in P0nlist:
			ConvertUnits(i,i,"Wavelength",AlignBins=1)
			Rebin(i,i,reb)
			removeoutlayer(i+"_1")
			removeoutlayer(i+"_2")
			CropWorkspace(i,i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
			if int(maxSpec) > int(minSpec):
				SumSpectra(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)
			else:
				CropWorkspace(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)
			Divide(i+"det",i+"mon",i+"norm")
			DeleteWorkspace(i+"mon")
			DeleteWorkspace(i+"det")
			DeleteWorkspace(i)
			Minus(i+"norm_"+upPeriod,i+"norm_"+downPeriod,"num")
			Plus(i+"norm_2",i+"norm_1","den")
			Divide("num","den",i+"pol")
			ReplaceSpecialValues(i+"pol",i+"pol",0.0,0.0,0.0,0.0)
			DeleteWorkspace(i+"norm_2")
			DeleteWorkspace(i+"norm_1")
			DeleteWorkspace("num")
			DeleteWorkspace("den")
		
	for i in range(len(nlist)):
		if existingP0 != "2":
			Divide(nlist[i]+"pol",P0nlist[i]+"pol",nlist[i]+"SESANS")
			if nspec > 4:
				Divide(nlist[i]+"2dpol",P0nlist[i]+"pol",nlist[i]+"2dSESANS")
		else:
			Divide(nlist[i]+"pol",P0nlist[i],nlist[i]+"SESANS")
			if nspec > 4:
				Divide(nlist[i]+"2dpol",P0nlist[i],nlist[i]+"2dSESANS")
		ReplaceSpecialValues(nlist[i]+"SESANS",nlist[i]+"SESANS",0.0,0.0,0.0,0.0)
	
	SEConstList=parseNameList(SEConstants)
	k=0
	for i in nlist:
		a1=mantid.getMatrixWorkspace(i+"SESANS")
		
		x=a1.readX(0)
		for j in range(len(x)-1):
			lam=((a1.readX(0)[j]+a1.readX(0)[j+1])/2.0)/10.0
			p=a1.readY(0)[j]
			e=a1.readE(0)[j]
			if lnPOverLam == "2":
				if p > 0.0:
					a1.dataY(0)[j]=log(p)/((lam*1.0e-9)**2)
					a1.dataE(0)[j]=(e/p)/((lam*1.0e-9)**2)
				else:
					a1.dataY(0)[j]=0.0
					a1.dataE(0)[j]=0.0
		for j in range(len(x)):
			if convertToSEL == "2":
				lam=a1.readX(0)[j]
				a1.dataX(0)[j]=1.0e-2*float(SEConstList[k])*lam*lam
				#print str(lam)+" "+str(1.0e-2*float(SEConstList[k])*lam*lam)
		k=k+1

	
	if nspec > 4:
		k=0
		for i in nlist:
			a1=mantid.getMatrixWorkspace(i+"2dSESANS")
			nspec=a1.getNumberHistograms()
			for l in range(nspec):
				x=a1.readX(l)
				for j in range(len(x)-1):
					lam=((a1.readX(l)[j]+a1.readX(l)[j+1])/2.0)/10.0
					p=a1.readY(l)[j]
					e=a1.readE(l)[j]
					if lnPOverLam == "2":
						if p > 0.0:
							a1.dataY(l)[j]=log(p)/((lam*1.0e-9)**2)
							a1.dataE(l)[j]=(e/p)/((lam*1.0e-9)**2)
						else:
							a1.dataY(l)[j]=0.0
							a1.dataE(l)[j]=0.0
				for j in range(len(x)):
					if convertToSEL == "2":
						lam=a1.readX(l)[j]
						a1.dataX(l)[j]=1.0e-2*float(SEConstList[k])*lam*lam
						#print str(lam)+" "+str(1.0e-2*float(SEConstList[k])*lam*lam)
			k=k+1

#
#===========================================================
#
def nrCalcSEConst(RFFrequency,poleShoeAngle):
	if (RFFrequency=="0.5"):
		B=0.53*34.288
	elif (RFFrequency=="1.0"):
		B=34.288
	else:
		B=2.0*34.288
	
	h=6.62607e-34
	m=1.67493e-27
	L=1.0
	Gl=1.83247e8
	#
	# correct the angle
	# calibration of th0 using gold grating Dec 2010
	#
	th0=float(poleShoeAngle)
	th0=-0.0000000467796*(th0**5)+0.0000195413*(th0**4)-0.00326229*(th0**3)+0.271767*(th0**2)-10.4269*th0+198.108
	c1=Gl*m*2.0*B*L/(2.0*pi*h*tan(th0*pi/180.0)*1.0e20)
	print c1*1e8
	return c1*1e8
#
#===========================================================
#
def nrSESANSP0Fn(P0runList,P0nameList,minSpec,maxSpec,upPeriod,downPeriod,gparams):

	P0nlist=parseNameList(P0nameList)
	mtd.sendLogMessage("This is the P0nameslist:"+str(P0nlist))
	P0rlist=parseRunList(P0runList)
	mtd.sendLogMessage("This is the P0runlist:"+str(P0rlist))
	for i in range(len(P0rlist)):
		addRuns(P0rlist[i],P0nlist[i])

	mon_spec=int(gparams[3])-1
	minSp=int(minSpec)-1
	maxSp=int(maxSpec)-1
	reb=gparams[0]+","+gparams[1]+","+gparams[2]
		
	for i in P0nlist:
		ConvertUnits(i,i,"Wavelength",AlignBins=1)
		Rebin(i,i,reb)
		CropWorkspace(i,i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
		if int(maxSpec) > int(minSpec):
			SumSpectra(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)
		else:
			CropWorkspace(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)
		Divide(i+"det",i+"mon",i+"norm")
		DeleteWorkspace(i+"mon")
		DeleteWorkspace(i+"det")
		DeleteWorkspace(i)
		Minus(i+"norm_"+upPeriod,i+"norm_"+downPeriod,"num")
		Plus(i+"norm_2",i+"norm_1","den")
		Divide("num","den",i+"pol")
		ReplaceSpecialValues(i+"pol",i+"pol",0.0,0.0,0.0,0.0)
		DeleteWorkspace(i+"norm_2")
		DeleteWorkspace(i+"norm_1")
		DeleteWorkspace("num")
		DeleteWorkspace("den")
#
#===========================================================
#
def nrSERGISFn(runList,nameList,P0runList,P0nameList,minSpec,maxSpec,upPeriod,downPeriod,existingP0,SEConstants,gparams,lnPOverLam):
	nlist=parseNameList(nameList)
	mtd.sendLogMessage("This is the sample nameslist:"+str(nlist))
	rlist=parseRunList(runList)
	mtd.sendLogMessage("This is the sample runlist:"+str(rlist))
	for i in range(len(rlist)):
		addRuns(rlist[i],nlist[i])

	P0nlist=parseNameList(P0nameList)
	mtd.sendLogMessage("This is the P0nameslist:"+str(P0nlist))
	if existingP0 != "2":
		P0rlist=parseRunList(P0runList)
		mtd.sendLogMessage("This is the P0runlist:"+str(P0rlist))
		for i in range(len(P0rlist)):
			addRuns(P0rlist[i],P0nlist[i])

	mon_spec=int(gparams[3])-1
	minSp=int(minSpec)-1
	maxSp=int(maxSpec)-1
	reb=gparams[0]+","+gparams[1]+","+gparams[2]
	for i in nlist:
		ConvertUnits(i,i,"Wavelength",AlignBins=1)
		Rebin(i,i,reb)
		CropWorkspace(i,i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
		if int(maxSpec) > int(minSpec):
			SumSpectra(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)
		else:
			CropWorkspace(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)

		Divide(i+"det",i+"mon",i+"norm")
		DeleteWorkspace(i+"mon")
		DeleteWorkspace(i+"det")
		DeleteWorkspace(i)
		Minus(i+"norm_"+upPeriod,i+"norm_"+downPeriod,"num")
		Plus(i+"norm_2",i+"norm_1","den")
		Divide("num","den",i+"pol")
		ReplaceSpecialValues(i+"pol",i+"pol",0.0,0.0,0.0,0.0)
		DeleteWorkspace(i+"norm_2")
		DeleteWorkspace(i+"norm_1")
		DeleteWorkspace("num")
		DeleteWorkspace("den")
		
	if existingP0 != "2":
		for i in P0nlist:
			ConvertUnits(i,i,"Wavelength",AlignBins=1)
			Rebin(i,i,reb)
			CropWorkspace(i,i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
			if int(maxSpec) > int(minSpec):
				SumSpectra(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)
			else:
				CropWorkspace(i,i+"det",StartWorkspaceIndex=minSp,EndWorkspaceIndex=maxSp)
			Divide(i+"det",i+"mon",i+"norm")
			DeleteWorkspace(i+"mon")
			DeleteWorkspace(i+"det")
			DeleteWorkspace(i)
			Minus(i+"norm_"+upPeriod,i+"norm_"+downPeriod,"num")
			Plus(i+"norm_2",i+"norm_1","den")
			Divide("num","den",i+"pol")
			ReplaceSpecialValues(i+"pol",i+"pol",0.0,0.0,0.0,0.0)
			DeleteWorkspace(i+"norm_2")
			DeleteWorkspace(i+"norm_1")
			DeleteWorkspace("num")
			DeleteWorkspace("den")
		
	for i in range(len(nlist)):
		if existingP0 != "2":
			Divide(nlist[i]+"pol",P0nlist[i]+"pol",nlist[i]+"SESANS")
		else:
			Divide(nlist[i]+"pol",P0nlist[i]+"pol",nlist[i]+"SESANS")
		ReplaceSpecialValues(nlist[i]+"SESANS",nlist[i]+"SESANS",0.0,0.0,0.0,0.0)
	
	SEConstList=parseNameList(SEConstants)
	k=0
	for i in nlist:
		a1=mantid.getMatrixWorkspace(i+"SESANS")
		x=a1.readX(0)
		for j in range(len(x)-1):
			lam=((a1.readX(0)[j]+a1.readX(0)[j+1])/2.0)/10.0
			p=a1.readY(0)[j]
			a1.dataY(0)[j]=log(p)/((lam*1.0e-8)**2)
		for j in range(len(x)):
			lam=a1.readX(0)[j]
			a1.dataX(0)[j]=1.0e-2*float(SEConstList[k])*lam*lam
			#print str(lam)+" "+str(1.0e-2*float(SEConstList[k])*lam*lam)
		k=k+1
#
#===========================================================
#
def nrNRFn(runList,nameList,incidentAngles,DBList,specChan,minSpec,maxSpec,gparams,floodfile):
	nlist=parseNameList(nameList)
	mtd.sendLogMessage("This is the sample nameslist:"+str(nlist))
	rlist=parseRunList(runList)
	mtd.sendLogMessage("This is the sample runlist:"+str(rlist))
	dlist=parseNameList(DBList)
	mtd.sendLogMessage("This is the Direct Beam nameslist:"+str(dlist))
	incAngles=parseNameList(incidentAngles)
	mtd.sendLogMessage("This incident Angles are:"+str(incAngles))

	for i in range(len(rlist)):
		addRuns(rlist[i],nlist[i])
	
	mon_spec=int(gparams[3])-1
	reb=gparams[0]+","+gparams[1]+","+gparams[2]
	
	k=0
	for i in nlist:
		if(mtd[i].isGroup()):
			#RenameWorkspace(i+"_1",i)
			snames=mtd[i].getNames()
			Plus(i+"_1",i+"_2","wtemp")
			if len(snames) > 2:
				for j in range(2,len(snames)-1):
					Plus("wtemp",snames[j],"wtemp")
			for j in snames:
				DeleteWorkspace(j)
			RenameWorkspace("wtemp",i)
		ConvertUnits(InputWorkspace=i,OutputWorkspace=i,Target="Wavelength",AlignBins="1")
		Rebin(InputWorkspace=i,OutputWorkspace=i,Params=reb)
		CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
		a1=mantid.getMatrixWorkspace(i)
		nspec=a1.getNumberHistograms()
		if nspec == 4:
			CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"det",StartWorkspaceIndex=3,EndWorkspaceIndex=3)
			RotateInstrumentComponent(i+"det","DetectorBench",X="-1.0",Angle=str(2.0*float(incAngles[k])))
			Divide(i+"det",i+"mon",i+"norm")
			if dlist[k] != "none":
				Divide(i+"norm",dlist[k],i+"norm")
				ReplaceSpecialValues(i+"norm",i+"norm","0.0","0.0","0.0","0.0")
				ConvertUnits(i+"norm",i+"RvQ",Target="MomentumTransfer")
		else:
			minSp=int(minSpec)
			maxSp=int(maxSpec)
			CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"det",StartWorkspaceIndex=4,EndWorkspaceIndex=243)
			floodnorm(i+"det",floodfile)
			# move the first spectrum in the list onto the beam centre so that when the bench is rotated it's in the right place
			MoveInstrumentComponent(i+"det","DetectorBench",Y=str((125.0-float(minSpec))*1.2e-3))
			# add a bit to the angle to put the first spectrum of the group in the right place
			a1=2.0*float(incAngles[k])+atan((float(minSpec)-float(specChan))*1.2e-3/3.53)*180.0/pi
			#print str(2.0*float(incAngles[k]))+" "+str(atan((float(minSpec)-float(specChan))*1.2e-3/3.63)*180.0/pi)+" "+str(a1)
			RotateInstrumentComponent(i+"det","DetectorBench",X="-1.0",Angle=str(a1))
			GroupDetectors(i+"det",i+"sum",WorkspaceIndexList=range(int(minSpec)-5,int(maxSpec)-5+1),KeepUngroupedSpectra="0")
			Divide(i+"sum",i+"mon",i+"norm")
			Divide(i+"det",i+"mon",i+"detnorm")
			if dlist[k]  == "none":
				a1=0
			elif dlist[k] == "function":
				# polynomial + power law corrections based on Run numbers 8291 and 8292
				Divide(i+'norm',i+'norm',i+'normt1')
				PolynomialCorrection(i+'normt1',i+'normPC','-0.0177398,0.00101695,0.0',Operation='Multiply')
				PowerLawCorrection(i+'normt1',i+'normPLC','2.01332','-1.8188')
				Plus(i+'normPC',i+'normPLC',i+'normt1')
				Divide(i+'norm',i+'normt1',i+'norm')
				ReplaceSpecialValues(i+'norm',i+'norm',0.0,0.0,0.0,0.0)
				DeleteWorkspace(i+'normPC')
				DeleteWorkspace(i+'normPLC')
				DeleteWorkspace(i+'normt1')
			else:
				Divide(i+"norm",dlist[k],i+"norm")
				ReplaceSpecialValues(i+"norm",i+"norm","0.0","0.0","0.0","0.0")
				Divide(i+"detnorm",dlist[k],i+"detnorm")
				ReplaceSpecialValues(i+"detnorm",i+"detnorm","0.0","0.0","0.0","0.0")
			ConvertUnits(i+"norm",i+"RvQ",Target="MomentumTransfer")
			#floodnorm(i+"detnorm",floodfile)
			DeleteWorkspace(i+"sum")
			
		k=k+1
		DeleteWorkspace(i)
		DeleteWorkspace(i+"mon")
		DeleteWorkspace(i+"det")

#
#===========================================================
#
def findbin(wksp,val):
	a1=mantid.getMatrixWorkspace(wksp)
	x1=a1.readX(0)
	bnum=-1
	for i in range(len(x1)-1):
		if x1[i] > val:
			break
	return i-1
#
#===========================================================
#
def nrDBFn(runListShort,nameListShort,runListLong,nameListLong,nameListComb,minSpec,maxSpec,minWavelength,gparams,floodfile=""):
	nlistS=parseNameList(nameListShort)
	rlistS=parseRunList(runListShort)
	nlistL=parseNameList(nameListLong)
	rlistL=parseRunList(runListLong)
	nlistComb=parseNameList(nameListComb)

	for i in range(len(rlistS)):
		addRuns(rlistS[i],nlistS[i])
	for i in range(len(rlistL)):
		addRuns(rlistL[i],nlistL[i])
	
	mon_spec=int(gparams[3])-1
	minSp=int(minSpec)-1
	maxSp=int(maxSpec)-1
	reb=gparams[0]+","+gparams[1]+","+gparams[2]
	
	for i in nlistS:
		ConvertUnits(InputWorkspace=i,OutputWorkspace=i,Target="Wavelength",AlignBins="1")
		Rebin(InputWorkspace=i,OutputWorkspace=i,Params=reb)
		CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
		if(mtd[i].isGroup()):
			snames=mtd[i].getNames()
			a1=mantid.getMatrixWorkspace(snames[0])
		else:
			a1=mantid.getMatrixWorkspace(i)
		
		nspec=a1.getNumberHistograms()
		
		if nspec == 4:
			CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"det",StartWorkspaceIndex=3,EndWorkspaceIndex=3)
			Divide(i+"det",i+"mon",i+"norm")
			ReplaceSpecialValues(i+"norm",i+"norm","0.0","0.0","0.0","0.0")
		else:
			CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"det",StartWorkspaceIndex=4,EndWorkspaceIndex=243)
			floodnorm(i+"det",floodfile)
			GroupDetectors(i+"det",i+"sum",WorkspaceIndexList=range(int(minSpec)-5,int(maxSpec)-5+1),KeepUngroupedSpectra="0")
			Divide(i+"sum",i+"mon",i+"norm")
			ReplaceSpecialValues(i+"norm",i+"norm","0.0","0.0","0.0","0.0")
			
	for i in nlistL:
		ConvertUnits(InputWorkspace=i,OutputWorkspace=i,Target="Wavelength",AlignBins="1")
		Rebin(InputWorkspace=i,OutputWorkspace=i,Params=reb)
		CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
		if(mtd[i].isGroup()):
			lnames=mtd[i].getNames()
			a1=mantid.getMatrixWorkspace(lnames[0])
		else:
			a1=mantid.getMatrixWorkspace(i)

		nspec=a1.getNumberHistograms()

		if nspec == 4:
			CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"det",StartWorkspaceIndex=3,EndWorkspaceIndex=3)
			Divide(i+"det",i+"mon",i+"norm")
			ReplaceSpecialValues(i+"norm",i+"norm","0.0","0.0","0.0","0.0")
		else:
			CropWorkspace(InputWorkspace=i,OutputWorkspace=i+"det",StartWorkspaceIndex=4,EndWorkspaceIndex=243)
			floodnorm(i+"det",floodfile)
			GroupDetectors(i+"det",i+"sum",WorkspaceIndexList=range(int(minSpec)-5,int(maxSpec)-5+1),KeepUngroupedSpectra="0")
			Divide(i+"sum",i+"mon",i+"norm")
			ReplaceSpecialValues(i+"norm",i+"norm","0.0","0.0","0.0","0.0")
		
	for i in range(len(nlistS)):
		if(mtd[nlistS[i]+"norm"].isGroup()):
			snames=mtd[nlistS[i]+"norm"].getNames()
			lnames=mtd[nlistL[i]+"norm"].getNames()
			for k in range(len(snames)):
				Integration(snames[k],snames[k]+"int",minWavelength,gparams[2])
				Integration(lnames[k],lnames[k]+"int",minWavelength,gparams[2])
				Multiply(snames[k],lnames[k]+"int",snames[k])
				Divide(snames[k],snames[k]+"int",snames[k])
				a1=findbin(lnames[k],float(minWavelength))
				MultiplyRange(lnames[k],lnames[k],"0",str(a1),"0.0")
				WeightedMean(snames[k],lnames[k],nlistComb[i]+"_"+str(k+1))
				#DeleteWorkspace(snames[k]+"int")
				#DeleteWorkspace(lnames[k]+"int")
		else:
			Integration(nlistS[i]+"norm",nlistS[i]+"int",minWavelength,gparams[2])
			Integration(nlistL[i]+"norm",nlistL[i]+"int",minWavelength,gparams[2])
			Multiply(nlistS[i]+"norm",nlistL[i]+"int",nlistS[i]+"norm")
			Divide(nlistS[i]+"norm",nlistS[i]+"int",nlistS[i]+"norm")
			a1=findbin(nlistL[i]+"norm",float(minWavelength))
			MultiplyRange(nlistL[i]+"norm",nlistL[i]+"norm","0",str(a1),"0.0")
			WeightedMean(nlistS[i]+"norm",nlistL[i]+"norm",nlistComb[i])
			#DeleteWorkspace(nlistS[i]+"int")
			#DeleteWorkspace(nlistL[i]+"int")
		#DeleteWorkspace(nlistS[i]+"mon")
		#DeleteWorkspace(nlistS[i]+"det")
		##DeleteWorkspace(nlistS[i]+"sum")
		#DeleteWorkspace(nlistS[i]+"norm")
		#DeleteWorkspace(nlistS[i])
		#DeleteWorkspace(nlistL[i]+"mon")
		#DeleteWorkspace(nlistL[i]+"det")
		##DeleteWorkspace(nlistL[i]+"sum")
		#DeleteWorkspace(nlistL[i]+"norm")
		#DeleteWorkspace(nlistL[i])
		
#
#===========================================================
#
def numberofbins(wksp):
	a1=mantid.getMatrixWorkspace(wksp)
	y1=a1.readY(0)
	return len(y1)-1
#
#===========================================================
#
def maskbin(wksp,val):
	a1=mantid.getMatrixWorkspace(wksp)
	x1=a1.readX(0)
	for i in range(len(x1)-1):
		if x1[i] > val:
			break
	a1.dataY(0)[i-1]=0.0
	a1.dataE(0)[i-1]=0.0
#
#===========================================================
#
def arr2list(iarray):
	# convert array of strings to a single string with commas
	res=""
	for i in range(len(iarray)-1):
		res=res+iarray[i]+","
	res=res+iarray[len(iarray)-1]
	return res
#
#===========================================================
#
def NRCombineDatafn(RunsNameList,CombNameList,applySFs,SFList,SFError,scaleOption,bparams,globalSF,applyGlobalSF):
	qmin=bparams[0]
	bin=bparams[1]
	qmax=bparams[2]
	rlist=parseNameList(RunsNameList)
	listsfs=parseNameList(SFList)
	listsfserr=parseNameList(SFError)
	sfs=[]
	sferrs=[]
	for i in rlist:
		Rebin(i,i+"reb",qmin+","+bin+","+qmax)
	# find the overlap ranges
	bol=[] #beginning of overlaps
	eol=[] #end of overlaps
	for i in range(len(rlist)-1):
		a1=mantid.getMatrixWorkspace(rlist[i+1])
		x=a1.readX(0)
		bol.append(x[0])
		a1=mantid.getMatrixWorkspace(rlist[i])
		x=a1.readX(0)
		eol.append(x[len(x)-1])
	# set the edges of the rebinned data to 0.0 to avoid partial bin problems
	maskbin(rlist[0]+"reb",eol[0])
	if len(rlist) > 2:
		for i in range(1,len(rlist)-1):
			maskbin(rlist[i]+"reb",bol[i-1])
			maskbin(rlist[i]+"reb",eol[i])
	maskbin(rlist[len(rlist)-1]+"reb",bol[len(rlist)-2])
	# Now find the various scale factors and store in temp workspaces
	for i in range(len(rlist)-1):
		Integration(rlist[i]+"reb","i"+str(i)+"1temp",str(bol[i]),str(eol[i]))
		Integration(rlist[i+1]+"reb","i"+str(i)+"2temp",str(bol[i]),str(eol[i]))
		if scaleOption != "2":
			Divide("i"+str(i)+"1temp","i"+str(i)+"2temp","sf"+str(i))
			a1=mantid.getMatrixWorkspace("sf"+str(i))
			print "sf"+str(i)+"="+str(a1.readY(0))+" +/- "+str(a1.readE(0))
			sfs.append(str(a1.readY(0)[0]))
			sferrs.append(str(a1.readE(0)[0]))
		else:
			Divide("i"+str(i)+"2temp","i"+str(i)+"1temp","sf"+str(i))
			print "sf"+str(i)+"="+str(a1.readY(0))+" +/- "+str(a1.readE(0))
			sfs.append(str(a1.readY(0)[0]))
			sferrs.append(str(a1.readE(0)[0]))
		mantid.deleteWorkspace("i"+str(i)+"1temp")
		mantid.deleteWorkspace("i"+str(i)+"2temp")
	# if applying pre-defined scale factors substitute the given values now 
	# Note the errors are now set to 0
	if applySFs == "2":
		for i in range(len(rlist)-1):
			a1=mantid.getMatrixWorkspace("sf"+str(i))
			a1.dataY(0)[0]=float(listsfs[i])
			a1.dataE(0)[0]=float(listsfserr[i])
	# Now scale the various data sets in the correct order
	if scaleOption != "2":
		for i in range(len(rlist)-1):
			for j in range(i+1,len(rlist)):
				Multiply(rlist[j]+"reb","sf"+str(i),rlist[j]+"reb")
	else:
		for i in range(len(rlist)-1,0,-1):
			for j in range(i,0,-1):
				Multiply(rlist[j]+"reb","sf"+str(i-1),rlist[j]+"reb")

	WeightedMean(rlist[0]+"reb",rlist[1]+"reb","currentSum")
	if len(rlist) > 2:
		for i in range(2,len(rlist)):
			WeightedMean("currentSum",rlist[i]+"reb","currentSum")
	
	# if applying a global scale factor do it here
	if applyGlobalSF == "2":
		scaledData=mtd['currentSum']/float(globalSF)
		RenameWorkspace('scaledData',CombNameList)
		mantid.deleteWorkspace('currentSum')
	else:
		RenameWorkspace('currentSum',CombNameList)
	for i in range(len(rlist)-1):
		mantid.deleteWorkspace("sf"+str(i))
	#for i in range(len(rlist)):
	#	mantid.deleteWorkspace(rlist[i]+"reb")
	return [arr2list(sfs),arr2list(sferrs)] 
	
#
#===========================================================
#
def nrWriteXYE(wksp,fname):
	a1=mantid.getMatrixWorkspace(wksp)
	x1=a1.readX(0)
	X1=n.zeros((len(x1)-1))
	for i in range(0,len(x1)-1):
		X1[i]=(x1[i]+x1[i+1])/2.0
	y1=a1.readY(0)
	e1=a1.readE(0)
	f=open(fname,'w')
	for i in range(len(X1)):
		s=""
		s+="%f," % X1[i]
		s+="%f," % y1[i]
		s+="%f\n" % e1[i]  
		f.write(s)
	f.close()
#
#===========================================================
#
def nrPNRCorrection(UpWksp,DownWksp):
	crho=[0.941893,0.0234006,-0.00210536,0.0]
	calpha=[0.945088,0.0242861,-0.00213624,0.0]
	cAp=[1.00079,-0.0186778,0.00131546,0.0]
	cPp=[1.01649,-0.0228172,0.00214626,0.0]
	Ip = mtd[UpWksp]
	Ia = mtd[DownWksp]
	CloneWorkspace(Ip,"PCalpha")
	CropWorkspace(InputWorkspace="PCalpha",OutputWorkspace="PCalpha",StartWorkspaceIndex="0",EndWorkspaceIndex="0")
	alpha=mtd['PCalpha']
	a1=alpha.readY(0)
	for i in range(0,len(a1)):
		alpha.dataY(0)[i]=0.0
		alpha.dataE(0)[i]=0.0
	CloneWorkspace("PCalpha","PCrho")
	CloneWorkspace("PCalpha","PCAp")
	CloneWorkspace("PCalpha","PCPp")
	rho=mtd['PCrho']
	Ap=mtd['PCAp']
	Pp=mtd['PCPp']
	for i in range(0,len(a1)):
		x=(alpha.dataX(0)[i]+alpha.dataX(0)[i])/2.0
		for j in range(0,4):
			alpha.dataY(0)[i]=alpha.dataY(0)[i]+calpha[j]*x**j
			rho.dataY(0)[i]=rho.dataY(0)[i]+crho[j]*x**j
			Ap.dataY(0)[i]=Ap.dataY(0)[i]+cAp[j]*x**j
			Pp.dataY(0)[i]=Pp.dataY(0)[i]+cPp[j]*x**j
	D=Pp*(1.0+rho)
	nIp=(Ip*(rho*Pp+1.0)+Ia*(Pp-1.0))/D
	nIa=(Ip*(rho*Pp-1.0)+Ia*(Pp+1.0))/D
	RenameWorkspace(nIp,str(Ip)+"corr")
	RenameWorkspace(nIa,str(Ia)+"corr")
	iwksp=mantid.getWorkspaceNames()
	list=[str(Ip),str(Ia),"PCalpha","PCrho","PCAp","PCPp","1_p"]
	for i in range(len(iwksp)):
		for j in list:
			lname=len(j)
			if iwksp[i] [0:lname+1] == j+"_":
				mantid.deleteWorkspace(iwksp[i])
	mantid.deleteWorkspace("PCalpha")
	mantid.deleteWorkspace("PCrho")
	mantid.deleteWorkspace("PCAp")
	mantid.deleteWorkspace("PCPp")
	mantid.deleteWorkspace("D")
#
#===========================================================
#
def nrPACorrection(UpUpWksp,UpDownWksp,DownUpWksp,DownDownWksp):
	crho=[0.941893,0.0234006,-0.00210536,0.0]
	calpha=[0.945088,0.0242861,-0.00213624,0.0]
	cAp=[1.00079,-0.0186778,0.00131546,0.0]
	cPp=[1.01649,-0.0228172,0.00214626,0.0]
	Ipp = mtd[UpUpWksp]
	Ipa = mtd[UpDownWksp]
	Iap = mtd[DownUpWksp]
	Iaa = mtd[DownDownWksp]
	CloneWorkspace(Ipp,"PCalpha")
	CropWorkspace(InputWorkspace="PCalpha",OutputWorkspace="PCalpha",StartWorkspaceIndex="0",EndWorkspaceIndex="0")
	alpha=mtd['PCalpha']
	a1=alpha.readY(0)
	for i in range(0,len(a1)):
		alpha.dataY(0)[i]=0.0
		alpha.dataE(0)[i]=0.0
	CloneWorkspace("PCalpha","PCrho")
	CloneWorkspace("PCalpha","PCAp")
	CloneWorkspace("PCalpha","PCPp")
	rho=mtd['PCrho']
	Ap=mtd['PCAp']
	Pp=mtd['PCPp']
	for i in range(0,len(a1)):
		x=(alpha.dataX(0)[i]+alpha.dataX(0)[i])/2.0
		for j in range(0,4):
			alpha.dataY(0)[i]=alpha.dataY(0)[i]+calpha[j]*x**j
			rho.dataY(0)[i]=rho.dataY(0)[i]+crho[j]*x**j
			Ap.dataY(0)[i]=Ap.dataY(0)[i]+cAp[j]*x**j
			Pp.dataY(0)[i]=Pp.dataY(0)[i]+cPp[j]*x**j
	A0 = (Iaa * Pp * Ap) + (Ap * Ipa * rho * Pp) + (Ap * Iap * Pp * alpha) + (Ipp * Ap * alpha * rho * Pp)
	A1 = Pp * Iaa
	A2 = Pp * Iap
	A3 = Ap * Iaa
	A4 = Ap * Ipa
	A5 = Ap * alpha * Ipp
	A6 = Ap * alpha * Iap
	A7 = Pp * rho  * Ipp
	A8 = Pp * rho  * Ipa
	D = Pp * Ap *( 1.0 + rho + alpha + (rho * alpha) )  
	nIpp = (A0 - A1 + A2 - A3 + A4 + A5 - A6 + A7 - A8 + Ipp + Iaa - Ipa - Iap) / D
	nIaa = (A0 + A1 - A2 + A3 - A4 - A5 + A6 - A7 + A8 + Ipp + Iaa - Ipa - Iap) / D
	nIpa = (A0 - A1 + A2 + A3 - A4 - A5 + A6 + A7 - A8 - Ipp - Iaa + Ipa + Iap) / D
	nIap = (A0 + A1 - A2 - A3 + A4 + A5 - A6 - A7 + A8 - Ipp - Iaa + Ipa + Iap) / D
	RenameWorkspace(nIpp,str(Ipp)+"corr")
	RenameWorkspace(nIpa,str(Ipa)+"corr")
	RenameWorkspace(nIap,str(Iap)+"corr")
	RenameWorkspace(nIaa,str(Iaa)+"corr")
	ReplaceSpecialValues(str(Ipp)+"corr",str(Ipp)+"corr",NaNValue="0.0",NaNError="0.0",InfinityValue="0.0",InfinityError="0.0")
	ReplaceSpecialValues(str(Ipp)+"corr",str(Ipp)+"corr",NaNValue="0.0",NaNError="0.0",InfinityValue="0.0",InfinityError="0.0")
	ReplaceSpecialValues(str(Ipp)+"corr",str(Ipp)+"corr",NaNValue="0.0",NaNError="0.0",InfinityValue="0.0",InfinityError="0.0")
	ReplaceSpecialValues(str(Ipp)+"corr",str(Ipp)+"corr",NaNValue="0.0",NaNError="0.0",InfinityValue="0.0",InfinityError="0.0")
	iwksp=mantid.getWorkspaceNames()
	list=[str(Ipp),str(Ipa),str(Iap),str(Iaa),"PCalpha","PCrho","PCAp","PCPp","1_p"]
	for i in range(len(iwksp)):
		for j in list:
			lname=len(j)
			if iwksp[i] [0:lname+1] == j+"_":
				mantid.deleteWorkspace(iwksp[i])
	mantid.deleteWorkspace("PCalpha")
	mantid.deleteWorkspace("PCrho")
	mantid.deleteWorkspace("PCAp")
	mantid.deleteWorkspace("PCPp")
	mantid.deleteWorkspace('A0')
	mantid.deleteWorkspace('A1')
	mantid.deleteWorkspace('A2')
	mantid.deleteWorkspace('A3')
	mantid.deleteWorkspace('A4')
	mantid.deleteWorkspace('A5')
	mantid.deleteWorkspace('A6')
	mantid.deleteWorkspace('A7')
	mantid.deleteWorkspace('A8')
	mantid.deleteWorkspace('D')
#
#===========================================================
#
def nrPNRFn(runList,nameList,incidentAngles,DBList,specChan,minSpec,maxSpec,gparams,floodfile,PNRwithPA,pnums,doCorrs):
	nlist=parseNameList(nameList)
	mtd.sendLogMessage("This is the sample nameslist:"+str(nlist))
	rlist=parseRunList(runList)
	mtd.sendLogMessage("This is the sample runlist:"+str(rlist))
	dlist=parseNameList(DBList)
	mtd.sendLogMessage("This is the Direct Beam nameslist:"+str(dlist))
	incAngles=parseNameList(incidentAngles)
	mtd.sendLogMessage("This incident Angles are:"+str(incAngles))
	
	if PNRwithPA == "2":
		nper=4
		mtd.sendLogMessage("PNRwithPA="+str(PNRwithPA))
		mtd.sendLogMessage(str(pnums))
	else:
		nper=2
	
	for i in range(len(rlist)):
		addRuns(rlist[i],nlist[i])
	
	mon_spec=int(gparams[3])-1
	minSp=int(minSpec)
	maxSp=int(maxSpec)
	reb=gparams[0]+","+gparams[1]+","+gparams[2]
	
	k=0
	for i in nlist:
		glist1=""
		glist2=""
		glist3=""
		for j in range(nper):
			wksp=i+"_"+pnums[j]
			ConvertUnits(InputWorkspace=wksp,OutputWorkspace=wksp,Target="Wavelength",AlignBins="1")
			Rebin(InputWorkspace=wksp,OutputWorkspace=wksp,Params=reb)
			#removeoutlayer(i+"_"+pnums[j])
			CropWorkspace(InputWorkspace=wksp,OutputWorkspace=wksp+"mon",StartWorkspaceIndex=mon_spec,EndWorkspaceIndex=mon_spec)
			a1=mantid.getMatrixWorkspace(wksp)
			nspec=a1.getNumberHistograms()
			if nspec == 4:
				CropWorkspace(InputWorkspace=wksp,OutputWorkspace=wksp+"det",StartWorkspaceIndex=3,EndWorkspaceIndex=3)
				RotateInstrumentComponent(wksp+"det","DetectorBench",X="-1.0",Angle=str(2.0*float(incAngles[k])))
				Divide(wksp+"det",wksp+"mon",wksp+"norm")
				if dlist[k] != "none":
					Divide(wksp+"norm",dlist[k],wksp+"norm")
					ReplaceSpecialValues(wksp+"norm",wksp+"norm","0.0","0.0","0.0","0.0")
					ConvertUnits(wksp+"norm",wksp+"RvQ",Target="MomentumTransfer")
			else:
				CropWorkspace(InputWorkspace=wksp,OutputWorkspace=wksp+"det",StartWorkspaceIndex=4,EndWorkspaceIndex=243)
				# move the first spectrum in the list onto the beam centre so that when the bench is rotated it's in the right place
				MoveInstrumentComponent(wksp+"det","DetectorBench",Y=str((125.0-float(minSpec))*1.2e-3))
				# add a bit to the angle to put the first spectrum of the group in the right place
				a1=2.0*float(incAngles[k])+atan((float(minSpec)-float(specChan))*1.2e-3/3.53)*180.0/pi
				#print str(2.0*float(incAngles[k]))+" "+str(atan((float(minSpec)-float(specChan))*1.2e-3/3.63)*180.0/pi)+" "+str(a1)
				RotateInstrumentComponent(wksp+"det","DetectorBench",X="-1.0",Angle=str(a1))
				GroupDetectors(wksp+"det",wksp+"sum",WorkspaceIndexList=range(int(minSpec)-5,int(maxSpec)-5+1),KeepUngroupedSpectra="0")
				Divide(wksp+"sum",wksp+"mon",wksp+"norm")
				Divide(wksp+"det",wksp+"mon",wksp+"detnorm")
				if dlist[k]  != "none":
					Divide(wksp+"norm",dlist[k],wksp+"norm")
					ReplaceSpecialValues(wksp+"norm",wksp+"norm","0.0","0.0","0.0","0.0")
					Divide(wksp+"detnorm",dlist[k],wksp+"detnorm")
					ReplaceSpecialValues(wksp+"detnorm",wksp+"detnorm","0.0","0.0","0.0","0.0")
				else:
					atemp=1
					# polynomial + power law corrections based on Run numbers 8291 and 8292
					# Divide(wksp+'norm',wksp+'norm',wksp+'normt1')
					# PolynomialCorrection(wksp+'normt1',wksp+'normPC','-0.0177398,0.00101695,0.0',Operation='Multiply')
					# PowerLawCorrection(wksp+'normt1',wksp+'normPLC','2.01332','-1.8188')
					# Plus(wksp+'normPC',wksp+'normPLC',wksp+'normt1')
					# Divide(wksp+'norm',wksp+'normt1',wksp+'norm')
					# ReplaceSpecialValues(wksp+'norm',wksp+'norm',0.0,0.0,0.0,0.0)
					# DeleteWorkspace(wksp+'normPC')
					# DeleteWorkspace(wksp+'normPLC')
					# DeleteWorkspace(wksp+'normt1')
				ConvertUnits(wksp+"norm",wksp+"RvQ",Target="MomentumTransfer")
				floodnorm(wksp+"detnorm",floodfile)
				DeleteWorkspace(wksp+"sum")
			DeleteWorkspace(wksp+"mon")
			DeleteWorkspace(wksp+"det")
			if j < nper-1:
				glist1=glist1+wksp+"norm,"
				glist2=glist2+wksp+"detnorm,"
				glist3=glist3+wksp+"RvQ,"
			else:
				glist1=glist1+wksp+"norm"
				glist2=glist2+wksp+"detnorm"
				glist3=glist3+wksp+"RvQ"
		if doCorrs == "2":
			if nper == 2:
				nrPNRCorrection(i+"_"+pnums[0]+"norm",i+"_"+pnums[1]+"norm")
				GroupWorkspaces(InputWorkspaces=i+"_"+pnums[0]+"normcorr,"+i+"_"+pnums[1]+"normcorr",OutputWorkspace=i+"normcorr")
				ConvertUnits(i+"_"+pnums[0]+"normcorr",i+"_"+pnums[0]+"corrRvQ",Target="MomentumTransfer")
				ConvertUnits(i+"_"+pnums[1]+"normcorr",i+"_"+pnums[1]+"corrRvQ",Target="MomentumTransfer")
				GroupWorkspaces(InputWorkspaces=i+"_"+pnums[0]+"corrRvQ,"+i+"_"+pnums[1]+"corrRvQ",OutputWorkspace=i+"corrRvQ")
				if nspec > 4:
					nrPNRCorrection(i+"_"+pnums[0]+"detnorm",i+"_"+pnums[1]+"detnorm")
					GroupWorkspaces(InputWorkspaces=i+"_"+pnums[0]+"detnormcorr,"+i+"_"+pnums[1]+"detnormcorr",OutputWorkspace=i+"detnormcorr")
			else:
				nrPACorrection(i+"_"+pnums[0]+"norm",i+"_"+pnums[1]+"norm",i+"_"+pnums[2]+"norm",i+"_"+pnums[3]+"norm")
				GroupWorkspaces(InputWorkspaces=i+"_"+pnums[0]+"normcorr,"+i+"_"+pnums[1]+"normcorr,"+i+"_"+pnums[2]+"normcorr,"+i+"_"+pnums[3]+"normcorr",OutputWorkspace=i+"normcorr")
				ConvertUnits(i+"_"+pnums[0]+"normcorr",i+"_"+pnums[0]+"corrRvQ",Target="MomentumTransfer")
				ConvertUnits(i+"_"+pnums[1]+"normcorr",i+"_"+pnums[1]+"corrRvQ",Target="MomentumTransfer")
				ConvertUnits(i+"_"+pnums[2]+"normcorr",i+"_"+pnums[2]+"corrRvQ",Target="MomentumTransfer")
				ConvertUnits(i+"_"+pnums[3]+"normcorr",i+"_"+pnums[3]+"corrRvQ",Target="MomentumTransfer")
				GroupWorkspaces(InputWorkspaces=i+"_"+pnums[0]+"corrRvQ,"+i+"_"+pnums[1]+"corrRvQ,"+i+"_"+pnums[2]+"corrRvQ,"+i+"_"+pnums[3]+"corrRvQ",OutputWorkspace=i+"corrRvQ")
				if nspec > 4:
					nrPACorrection(i+"_"+pnums[0]+"detnorm",i+"_"+pnums[1]+"detnorm",i+"_"+pnums[2]+"detnorm",i+"_"+pnums[3]+"detnorm")
					GroupWorkspaces(InputWorkspaces=i+"_"+pnums[0]+"detnormcorr,"+i+"_"+pnums[1]+"detnormcorr,"+i+"_"+pnums[2]+"detnormcorr,"+i+"_"+pnums[3]+"detnormcorr",OutputWorkspace=i+"detnormcorr")
		if nper == 2:
			Minus(i+"_"+pnums[0]+"norm",i+"_"+pnums[1]+"norm","num")
			Plus(i+"_"+pnums[0]+"norm",i+"_"+pnums[1]+"norm","den")
			Divide("num","den",i+"lampol")
			ReplaceSpecialValues(i+"lampol",i+"lampol","0.0","0.0","0.0","0.0")
			Minus(i+"_"+pnums[0]+"RvQ",i+"_"+pnums[1]+"RvQ","num")
			Plus(i+"_"+pnums[0]+"RvQ",i+"_"+pnums[1]+"RvQ","den")
			Divide("num","den",i+"Qpol")
			ReplaceSpecialValues(i+"Qpol",i+"Qpol","0.0","0.0","0.0","0.0")
			Minus(i+"_"+pnums[0]+"detnorm",i+"_"+pnums[1]+"detnorm","num")
			Plus(i+"_"+pnums[0]+"detnorm",i+"_"+pnums[1]+"detnorm","den")
			Divide("num","den",i+"2Dpol")
			ReplaceSpecialValues(i+"2Dpol",i+"2Dpol","0.0","0.0","0.0","0.0")
			DeleteWorkspace("num")
			DeleteWorkspace("den")
		GroupWorkspaces(InputWorkspaces=glist1,OutputWorkspace=i+"norm")
		GroupWorkspaces(InputWorkspaces=glist3,OutputWorkspace=i+"RvQ")
		if nspec != 4:
			GroupWorkspaces(InputWorkspaces=glist2,OutputWorkspace=i+"detnorm")
		k=k+1
		DeleteWorkspace(i)
#
#===========================================================
#
def tl(wksp,th0,schan):
	pixel=1.2
	dist=3630
	ThetaInc=th0*pi/180.0
	a1=mantid.getMatrixWorkspace(wksp)
	y=a1.readY(0)
	ntc=len(y)
	nspec=a1.getNumberHistograms()
	x1=n.zeros((nspec+1,ntc+1))
	theta=n.zeros((nspec+1,ntc+1))
	y1=n.zeros((nspec,ntc))
	e1=n.zeros((nspec,ntc))
	for i in range(0,nspec):
		x=a1.readX(i)
		y=a1.readY(i)
		e=a1.readE(i)
		x1[i,0:ntc+1]=x[0:ntc+1]
		theta[i,:]=atan2( (i - schan-0.5) * pixel + dist * tan(ThetaInc) , dist)*180/pi
		y1[i,0:ntc]=y[0:ntc]
		e1[i,0:ntc]=e[0:ntc]
	x1[nspec,:]=x1[nspec-1,:]
	theta[nspec,:]=atan2( (nspec - schan-0.5) * pixel + dist * tan(ThetaInc) , dist)*180/pi
	d1=[x1,theta,y1,e1]
	return d1
#
#===========================================================
#
def writemap_tab(dat,th0,spchan,fname):
	a1=tl(dat,th0,spchan)
	f=open(fname,'w')
	x=a1[0]
	y=a1[1]
	z=a1[2]
	e=a1[3]
	s="\t"
	for i in range(0,n.shape(z)[1]-1):
		s+="%g\t" % ((x[0][i]+x[0][i+1])/2.0)
		s+="%g\t" % ((x[0][i]+x[0][i+1])/2.0)
	s+="\n"
	f.write(s)
	for i in range(0,n.shape(y)[0]-1):
		s=""
		s+="%g\t" % ((y[i][0]+y[i+1][0])/2.0)
		for j in range(0,n.shape(z)[1]-1):
			s+="%g\t" % z[i][j]
			s+="%g\t" % e[i][j]
		s+="\n"
		f.write(s)
	f.close()
#
#===========================================================
#
def xye(wksp):
	a1=mantid.getMatrixWorkspace(wksp)
	x1=a1.readX(0)
	X1=n.zeros((len(x1)-1))
	for i in range(0,len(x1)-1):
		X1[i]=(x1[i]+x1[i+1])/2.0
	y1=a1.readY(0)
	e1=a1.readE(0)
	d1=[X1,y1,e1]
	return d1
#
#======================================================================================
#
def writeXYE_tab(dat,fname):
	a1=xye(dat)
	f=open(fname,'w')
	x=a1[0]
	y=a1[1]
	e=a1[2]		
	s=""
	s+="x\ty\te\n"
	f.write(s)
	for i in range(len(x)):
		s=""
		s+="%f\t" % x[i]
		s+="%f\t" % y[i]
		s+="%f\n" % e[i]  
		f.write(s)
	f.close()


'''
def quickPlot(runlist,dataDir,lmin,reb,lmax,spmin,spmax,output,plotper,polper,zmin,zmax,zlog):

	isisDataDir=dataDir
	mtd.sendLogMessage("setting dataDir="+dataDir+" "+isisDataDir)
	deleteFromRootName(output)
	addruns(runlist,output)
	nper=nperFromList(output)
	rebpars=str(lmin)+","+str(reb)+","+str(lmax)
		
	if nper == 1:
		ConvertUnits(InputWorkspace=output,OutputWorkspace=output,Target="Wavelength",AlignBins="1")
		Rebin(InputWorkspace=output,OutputWorkspace=output,Params=rebpars)
		CropWorkspace(InputWorkspace=output,OutputWorkspace=output+"m",StartWorkspaceIndex="1",EndWorkspaceIndex="1")
		CropWorkspace(InputWorkspace=output,OutputWorkspace=output+"d",StartWorkspaceIndex=str(spmin+4),EndWorkspaceIndex=str(spmax+4))
		Divide(output+"d",output+"m",output+"n")
		workspace_mtx=qti.app.mantidUI.importMatrixWorkspace(output+"n")
		gr2d=workspace_mtx.plotGraph2D()
		l=gr2d.activeLayer()
		if zlog == 0:
			l.setAxisScale(1,zmin,zmax,0)
		else:
			l.setAxisScale(1,zmin,zmax,1)
	else:
		workspace_mtx=[]
		nplot=0
		for i in plotper:
			ConvertUnits(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i),Target="Wavelength",AlignBins="1")
			Rebin(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i),Params=rebpars)
			CropWorkspace(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i)+"m",StartWorkspaceIndex="1",EndWorkspaceIndex="1")
			CropWorkspace(InputWorkspace=output+"_"+str(i),OutputWorkspace=output+"_"+str(i)+"d",StartWorkspaceIndex=str(spmin+4),EndWorkspaceIndex=str(spmax+4))
			Divide(output+"_"+str(i)+"d",output+"_"+str(i)+"m",output+"_"+str(i)+"n")
			workspace_mtx.append(qti.app.mantidUI.importMatrixWorkspace(output+"_"+str(i)+"n"))
			gr2d=workspace_mtx[nplot].plotGraph2D()
			nplot=nplot+1
			l=gr2d.activeLayer()
			if zlog == 0:
				l.setAxisScale(1,zmin,zmax,0)
			else:
				l.setAxisScale(1,zmin,zmax,1)
		
		up=mtd[output+"_2d"]
		down=mtd[output+"_1d"]
		asym=(up-down)/(down+up)
		RenameWorkspace(asym,output+"_asym")
		ReplaceSpecialValues(output+"_asym",output+"_asym","0.0","0.0","0.0","0.0")
		workspace_mtx.append(qti.app.mantidUI.importMatrixWorkspace(output+"_asym"))
		gr2d=workspace_mtx[nplot].plotGraph2D()
		l=gr2d.activeLayer()
		l.setAxisScale(1,-1.0,1.0,0)


	mtd.sendLogMessage("quickPlot Finished")
'''
