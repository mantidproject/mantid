#pylint: disable=invalid-name
# MUSIC : Version of Minus for MIDAS

from IndirectImport import *
if is_supported_f2py_platform():
    muscat = import_f2py("muscat")
else:
    unsupported_message()

from mantid.simpleapi import *
from mantid import config, logger, mtd
from IndirectCommon import *
import sys, platform, math, os.path, numpy as np
mp = import_mantidplot()

def CalcW0(nq,dq,disp,coeff):
    Q = []
    Q2 = []
    w0 = []
    e0 = []
    for n in range(0,nq):
        q0 = (n+1)*dq
        Q.append(q0)
        q02 = q0*q0
        Q2.append(q02)
        if (disp == 'Poly'):
            w = coeff[0]+coeff[1]*q0+coeff[2]*q02+coeff[3]*q0*q02+coeff[4]*q02*q02
        if (disp == 'CE'):
            qk = coeff[1]*q0
            w = coeff[0]*(1.0-math.sin(qk)/qk)
        if (disp == 'SS'):
            w0 = coeff[0]*(1.0-math.exp(coeff[1]*q02))
        w0.append(w*0.001)           #convert from mmeV to meV
        e0.append(0.0)
    return Q,w0,e0

def CalcSqw(q0,nw2,nel,dw,w0):
    PKHT=1.0/math.pi
    xSqw = []
    ySqw = []
    eSqw = []
    nq = len(q0)
    Qaxis = ''
    for i in range(0,nq):
        ww0 = w0[i]
        for j in range(0,nw2):
#        S(I,J)=S(I,J)+PKHT*W0(I)/(W0(I)**2+W**2)  !Lorentzian S(Q,w)
            ww = (j-nel)*dw
            xSqw.append(ww)                  #convert to microeV
            ss = PKHT*ww0/(ww0**2+ww**2)
            ySqw.append(ss)
            eSqw.append(0.0)
        if i == 0:
            Qaxis += str(q0[i])
        else:
            Qaxis += ','+str(q0[i])
    CreateWorkspace(OutputWorkspace='S(Q,w)', DataX=xSqw, DataY=ySqw, DataE=eSqw,\
    	Nspec=nq, UnitX='DeltaE', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)

def CheckCoeff(disp,coeff):
    if (disp == 'CE') or (disp == 'SS'):
        if coeff[0] < 1e-8:
            error = disp + ' coeff 1 is zero'
            logger.notice('ERROR *** '+error)
            sys.exit(error)
        if coeff[1] < 1e-8:
            error = disp + ' coeff 2 is zero'
            logger.notice('ERROR *** '+error)
            sys.exit(error)
    if (disp == 'Poly'):
        cc = coeff[0]+coeff[1]+coeff[2]+coeff[3]+coeff[4]
        if cc < 1e-8:
            error = 'Poly coeffs all zero'
            logger.notice('ERROR *** '+error)
            sys.exit(error)

def CheckQw(grid):
    nq = grid[0]
    if nq == 0:
        error = 'Grid : Number of Q values is zero'
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    nw = grid[2]
    if nw == 0:
        error = 'Grid : Number of w values is zero'
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    dq = grid[1]
    if dq < 1e-5:
        error = 'Grid : Q increment is zero'
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    dw = grid[3]*0.001
    if dw < 1e-8:
        error = 'Grid : w increment is zero'
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    return nq,dq,nw,dw

def CreateSqw(disp,coeff,grid,Verbose):
    CheckCoeff(disp,coeff)
    if Verbose:
        logger.notice('Dispersion is '+disp)
        logger.notice('Coefficients : '+str(coeff))
    nq,dq,nw,dw = CheckQw(grid)
    q0,w0,e0 = CalcW0(nq,dq,disp,coeff)
    CreateWorkspace(OutputWorkspace=disp, DataX=q0, DataY=w0, DataE=e0,\
    	Nspec=1, UnitX='MomentumTransfer')
    nw2 = 2*nw+1
    nel= nw+1
    CalcSqw(q0,nw2,nel,dw,w0)

def ReadSqw(sqw,Verbose):
    logger.notice('Reading S(q,w) from workspace : '+sqw)
    nq,nw = CheckHistZero(sqw)
    axis = mtd[sqw].getAxis(1)
    Q = []
    for i in range(0,nq):
        Q.append(float(axis.label(i)))
    Q_in = PadArray(Q,500)
    Sqw_in = []
    for n in range(0,nq):
        Xw = mtd[sqw].readX(n)             # energy array
        Ys = mtd[sqw].readY(n)             # S(q,w) values
        Ys = PadArray(Ys,1000)          # pad to Fortran energy size 1000
        Sqw_in.append(Ys)
    dw = Xw[2]-Xw[1]
    if dw < 1e-8:
        error = 'Sqw : w increment is zero'
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    nel= nw+1
    for n in range(0,nw):
        if (Xw[n] < dw):
            nel = n
    dq = Q[1]-Q[0]
    if dq < 1e-5:
        error = 'Sqw : Q increment is zero'
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    if Verbose:
        logger.notice('Q : '+str(nq)+' points from '+str(Q[0])+' to '+str(Q[nq-1])+' at '+str(dq))
        logger.notice('w : '+str(nw)+' points from '+str(Xw[0])+' to '+str(Xw[nw])+' at '+str(dw)\
    		+' ; Elastic energy at : '+str(nel))
    X0 = []
    X0 = PadArray(X0,1000)              # zeroes
    for n in range(nq,500):                 # pad to Fortran Q size 500
        Sqw_in.append(X0)
    return nq,dq,Q_in,nw,dw,nel,Xw,Sqw_in

def CheckNeut(neut):
#    neut = [NRUN1, NRUN2, JRAND, MRAND, NMST]
    if neut[0] == 0:
        error = 'NRUN1 is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if neut[1] == 0:
        error = 'NRUN2 is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if neut[4] == 0:
        error = 'Number scatterings is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)

def CheckBeam(beam):
#    beam = [THICK, WIDTH, HEIGHT, alfa]
    if beam[0] <1e-5:
        error = 'Beam thickness is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if beam[1] <1e-5:
        error = 'Beam width is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if beam[2] <1e-5:
        error = 'Beam height is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)

def CheckSam(sam):
    if sam[1] <1e-8:
        error = 'Sample density is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if (sam[2]+sam[3]) <1e-8:
        error = 'Sample total scattering cross-section (scat+abs) is Zero'
        logger.notice('ERROR *** ' + error)
        sys.exit(error)

def MuscatRun(sname,geom,neut,beam,sam,sqw,kr1,Verbose,Plot,Save):
#    neut = [NRUN1, NRUN2, JRAND, MRAND, NMST]
#    beam = [THICK, WIDTH, HEIGHT, alfa]
#   sam = [temp, dens, siga, sigb]
    workdir = config['defaultsave.directory']
    hist,npt = CheckHistZero(sname)
    CheckNeut(neut)
    CheckBeam(beam)
    CheckSam(sam)
    samWS = sname
    efixed = getEfixed(samWS)
    angle,QQ = GetThetaQ(samWS)
    mang = len(angle)
#   ijeom = [jeom, Jann]
    Jann = 1
    if geom == 'Flat':
        ijeom = [2, Jann]
    if geom == 'Cyl':
        ijeom = [3, Jann]
#   rgeom = [thick, width, height]
    rgeom = [beam[0], beam[1], beam[2]]
    nran = [neut[0], neut[1], neut[2], neut[3]]
    lpt = os.path.join(workdir, sname[:-3]+geom+'_ms.lpt')		# path name for lpt file
    if Verbose:
        logger.notice('Detectors/angles : '+str(mang))
        logger.notice('Sample geometry : '+geom)
        logger.notice('Sample parameters')
        logger.notice('  sigs = '+str(sam[3])+' ; siga = '+str(sam[1])+' ; rho = '+str(sam[0]))
        if geom == 'Cyl':
            logger.notice('  inner radius = '+str(beam[0])+' ; outer radius = '+str(beam[1]))
        if geom == 'Flat':
            logger.notice('  thickness = '+str(beam[0]))
        logger.notice('Output lptfile : ' + lpt)
    llpt = len(lpt)
    lpt.ljust(140,' ')
    lsqw = len(sqw)
    nq,dq,Q_in,nw,dw,nel,Xw,Sqw_in = ReadSqw(sqw,Verbose)
#   ims = [NMST, NQ, NW, Nel, KR1]
    nmst = neut[4]
    ims = [neut[4], nq, nw, nel, 1]
    nw2 = 2*ims[2]+1
#   dqw = [DQ, DW]
    dqw = [dq, dw]
    sname = sname[:-4]
    ySin = []
    Qaxis = ''
    for m in range(0,mang):
#     rinstr = [efixed, theta, alfa]
        rinstr = [efixed, angle[m], beam[3]]
        logger.notice('Detector ' +str(m+1)+ ' at angle ' +str(angle[m])+ ' and Q = ' +str(QQ[m]))
#      SUBROUTINE MUSCAT_data(IDET,sfile,l_sf,rfile,l_rf,rinstr,nran,
#     1 ijeom,rgeom,sam,ims,dqw,Q_in,S_in,
#     2 totals,iw,energy,scat1,scatm,RR,S_out)
        idet = m+1
        kill,totals,iw,energy,scat1,scatm,RR,Sqw=muscat.muscat_data(idet,lpt,llpt,sqw,lsqw,rinstr,nran,\
    							ijeom,rgeom,sam,ims,dqw,Q_in,Sqw_in)
        if (kill != 0):
            error = 'Muscat error code : '+str(kill)
            logger.notice(error)
            sys.exit(error)
        else:
            xEn = energy[:iw]
            xEn = np.append(xEn,2*energy[iw-1]-energy[iw-2])
            ySc1 = scat1[:iw]              # single scattering energy distribution
            yScM = scatm[:iw]               # total scattering energy distribution
            yRr = RR[:iw]                   # R-factor energy distribution
            if m == 0:
                tot1 = np.array(totals[1])                 # total single scattering
                tot2 = np.array(totals[2])                 # total second scattering
                tot3 = np.array(totals[3])                 # total third scattering
                total = np.array(totals[4])                # total all scattering
                xMs = xEn
                yMsc1 = ySc1
                yMscM = yScM
                yMr = yRr
            else:
                tot1 = np.append(tot1,totals[1])
                tot2 = np.append(tot2,totals[2])
                tot3 = np.append(tot3,totals[3])
                total = np.append(total,totals[4])
                xMs = np.append(xMs,xEn)
                yMsc1 = np.append(yMsc1,ySc1)
                yMscM = np.append(yMscM,yScM)
                yMr = np.append(yMr,yRr)
                Qaxis += ','
            Qaxis += str(QQ[m])
# start output of Totals
    totx = [angle[0]]
    tote = np.zeros(mang)
    for m in range(1,mang):
        totx = np.append(totx,angle[m])
    nt = 1
    Taxis = 'Scat1'
    xTot = totx
    yTot = tot1
    eTot = tote
    spec_list = [nt-1]
    if nmst > 1:
        nt += 1
        spec_list.append(nt-1)
        Taxis += ',Scat2'
        xTot = np.append(xTot,totx)
        yTot = np.append(yTot,tot2)
        eTot = np.append(eTot,tote)
        if nmst > 2:
            nt += 1
            spec_list.append(nt-1)
            Taxis += ',Scat3'
            xTot = np.append(xTot,totx)
            yTot = np.append(yTot,tot3)
            eTot = np.append(eTot,tote)
    xTot = np.append(xTot,totx)
    yTot = np.append(yTot,total)
    eTot = np.append(eTot,tote)
    nt += 1
    spec_list.append(nt-1)
    Taxis += ',Total'
    logger.notice('nt : ' + str(nt))
    logger.notice('Taxis : ' + Taxis)
    logger.notice('Qaxis : ' + Qaxis)
    logger.notice('yTot : ' + str(len(yTot)))
    logger.notice('eTot : ' + str(len(eTot)))
    msname = sname+'_MS'
    CreateWorkspace(OutputWorkspace=msname+'_Totals', DataX=xTot, DataY=yTot, DataE=eTot,\
    	Nspec=nt, UnitX='MomentumTransfer')
#    	Nspec=nt, UnitX='MomentumTransfer', VerticalAxisUnit='Text', VerticalAxisValues='Taxis')
# start output of MultScat
    eMs = np.zeros(iw*mang)
    CreateWorkspace(OutputWorkspace=msname+'_1', DataX=xMs, DataY=yMsc1, DataE=eMs,\
    	Nspec=mang, UnitX='DeltaE', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
    CreateWorkspace(OutputWorkspace=msname+'_M', DataX=xMs, DataY=yMscM, DataE=eMs,\
    	Nspec=mang, UnitX='DeltaE', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
    CreateWorkspace(OutputWorkspace=msname+'_R', DataX=xMs, DataY=yMr, DataE=eMs,\
    	Nspec=mang, UnitX='DeltaE', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=Qaxis)
    group = msname+'_1,'+ msname+'_M,'+ msname+'_R'
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=msname+'_Scat')
# start output
    if Save:
        tot_path = os.path.join(workdir,msname+'_Totals.nxs')
        SaveNexusProcessed(InputWorkspace=msname+'_Totals', Filename=tot_path)
        scat_path = os.path.join(workdir,msname+'_Scat.nxs')
        SaveNexusProcessed(InputWorkspace=msname+'_Scat', Filename=scat_path)
        if Verbose:
            logger.notice('Output total scattering file : ' + tot_path)
            logger.notice('Output MS scattering file : ' + scat_path)
    if Plot:
        plotMuscat(msname,spec_list,Plot)

def MuscatFuncStart(sname,geom,neut,beam,sam,grid,disp,coeff,kr1,Verbose,Plot,Save):
    StartTime('Muscat Function')
    workdir = config['defaultsave.directory']
    sname = sname+'_red'
    spath = os.path.join(workdir, sname+'.nxs')		# path name for sample nxs file
    LoadNexusProcessed(FileName=spath, OutputWorkspace=sname)
    sqw = 'S(Q,w)'
    CreateSqw(disp,coeff,grid,Verbose)
    if Verbose:
        logger.notice('Sample run : '+spath)
        logger.notice('S(Q,w) from : '+disp)
    MuscatRun(sname,geom,neut,beam,sam,sqw,kr1,Verbose,Plot,Save)
    EndTime('Muscat Function')

def MuscatDataStart(sname,geom,neut,beam,sam,sqw,kr1,Verbose,Plot,Save):
    StartTime('Muscat Data')
    workdir = config['defaultsave.directory']
    sname = sname+'_red'
    spath = os.path.join(workdir, sname+'.nxs')		# path name for sample nxs file
    LoadNexusProcessed(FileName=spath, OutputWorkspace=sname)
    sqw = sqw+'_sqw'
    qpath = os.path.join(workdir, sqw+'.nxs')		# path name for S(Q,w) nxs file
    LoadNexusProcessed(FileName=qpath, OutputWorkspace=sqw)
    if Verbose:
        logger.notice('Sample run : '+spath)
        logger.notice('S(Q,w) file : '+qpath)
    MuscatRun(sname,geom,neut,beam,sam,sqw,kr1,Verbose,Plot,Save)
    EndTime('Muscat Data')

def plotMuscat(inWS,spec_list,Plot):
    if (Plot == 'Totals' or Plot == 'All'):
        tot_plot=mp.plotSpectrum(inWS+'_Totals',spec_list)
    if (Plot == 'Scat1' or Plot == 'All'):
        mp.importMatrixWorkspace(inWS+'_1').plotGraph2D()
