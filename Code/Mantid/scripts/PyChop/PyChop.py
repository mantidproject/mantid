#pylint: disable=invalid-name
import time as time
import math
import numpy
from mantid.simpleapi import *
try:
    from mantidplot import *
except ImportError:
    pass
'''
chop(inst,ei,chop_type,frequency):
python implementation of CHOP ver 1.0
simulates flux and resolution for the chopper instruments
MAPS MARI and HET
original FORTRAN by TGP
matlab version JWT


Chop can either be run from the command line in matlab
chop(inst,ei,chop_type,frequency)
either
chop('mari',single or vector,'s',single or vector for frequency)
or
chop('mari',single or vector for energy,'s','12s')
requires
ei = incident energy
chop_type -- type of chopper
frequency
inst type
'''
    #first some instrument parameters
    #Instument details

global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata

def setchoptype(inst_name,type):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata,instname,chop_type,chop_par

    error_message = 'Chopper type is not supported for '

    chop_type=type
    if inst_name=='mari' or inst_name=='mar' or inst_name=='MAR'or inst_name=='MARI':
        instname='mar'
        print 'setup for MARI'
        err = 0
        if type == 'c':
            chop_par=[1.520, 0.550, 49.00,  580.00, 0.00, 0.00]
            print 'MARI C (100meV) chopper chosen'
            titledata='MARI C (100meV)'
        elif type == 'g':
            chop_par=[.38, 0.02, 10.00,  800.00, 0.00, 0.00]
            print 'MARI G (Gd pack) chopper chosen'
            titledata='MARI G (Gd pack)'
        elif type == 's':
            chop_par=[2.280, 0.550, 49.00, 1300.00, 0.00, 0.00]
            print 'MARI S (sloppy) chopper chosen'
            titledata='MARI S (sloppy)'
        elif type == 'b':
            chop_par= [1.140, 0.550, 49.00,  820.00, 0.00, 0.00]
            print 'MARI B (200meV) chopper chosen'
            titledata='MARI B (200meV)'
        elif type == 'a':
            chop_par= [0.760, 0.550, 49.00, 1300.00, 0.00, 0.00]
            print 'MARI A (500meV) chopper chosen'
            titledata='MARI A (500meV)'
        elif chop_type == 'r':
            par= [1.143, 0.550, 49.00, 1300, 0.00, 0.00]
            print 'MARI R (500meV) chopper chosen'
            titledata='MARI R (500meV)'
        else:
            err_mess=error_message+instname
            print err_mess
            return (err_mess,1)

    elif inst_name=='maps' or inst_name=='map' or inst_name=='MAP'or inst_name=='MAPS':
        instname='map'
        print 'setup for MAPS'
        if type == 's':
            chop_par=[2.8990,0.5340,49.00, 1300.00, 0.00, 0.00]
            print 'MAPS S (sloppy) chopper chosen'
            titledata='MAPS S (sloppy)'
        elif type == 'b':
            chop_par= [ 1.8120,0.5340,49.00,  920.00, 0.00, 0.0]
            print 'MAPS B (200meV) chopper chosen'
            titledata='MAPS B (200meV)'
        elif type == 'a':
            chop_par= [1.0870,0.5340,49.00, 1300.00, 0.00, 0.00]
            print 'MAPS A (500meV) chopper chosen'
            titledata='MAPS A (500meV)'
        else:
            #type == 'c':
            #print 'MAPS C (100meV) not available'
            #return
            #titledata='MAPS C (100meV)'
            err=error_message+instname
            print err
            return (err,1)

    elif inst_name=='MER' or inst_name=='mer' or inst_name=='MERLIN'or inst_name=='merlin':
        instname='mer'
        print 'setup for MERLIN'
        if type == 'c':
            chop_par=[ 1.710, 0.550, 49.00,  580.00, 0.00, 0.00 ]
            print 'HET C (100meV) chopper chosen'
            titledata='HET C (100meV)'
        elif type == 'd':
            chop_par=[ 1.520, 0.550, 49.00,  410.00, 0.00, 0.00 ]
            print 'HET D (50meV) chopper chosen'
            titledata='HET D (50meV)'
        elif type == 's':
            chop_par=[2.280, 0.550, 49.00, 1300.00, 0.00, 0.00 ]
            print 'HET S (sloppy) chopper chosen'
            titledata='HET S (sloppy)'
        elif type == 'b':
            chop_par= [1.290, 0.550, 49.00,  920.00, 0.00, 0.00 ]
            print 'HET B (200meV) chopper chosen'
            titledata='HET B (200meV)'
        elif type == 'a':
            chop_par= [0.760, 0.550, 49.00, 1300.00, 0.00, 0.0 ]
            print 'HET A (500meV) chopper chosen'
            titledata='HET A (500meV)'
        else:
            err=error_message+instname
            print err
            return (err,1)
    else:
        return ("",0)

    return ("",0)


def calculate(ei,frequency,**kwargs):
    """
    calculate flux elastic line resolution and resolution as a fucntion of energy transfer
    calculate(ei,frequency,**kwargs)
    keywords
    all=True :      return alll variables
                van_el,van,flux=calculate(ei,frequency,all)
                integer flux
                elasictic line resolution float
                resolution as a fucntion of eTrans list
    resolution=True: return just a list of the resolution as a fucntion of energy transfer
                resolution=calculate(ei,frequency,resolution)
    flux=True:      returns flux calculation and elastic line resolution
    freq_dep=True:  Returns a list of flux as a function of Fermi speed from 50:600
    Freq_dep=false  Does speed dependent calculation but returns no output
    plot=True       Plot in mantidplot either speed dependence or resolution
    example:
    PyChop.calculate(10,100,freq_dep=False)
    """
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, instname,titledata,chop_par,chop_type


    if instname=='mari' or instname=='mar' or instname=='MAR'or instname=='MARI':
        #For mari
        x0 = 10.00      # ***GET CORRECT VALUE
        xa = 7.190      # ***GET CORRECT VALUE
        x1 = 1.7390
        x2 = 4.0220
        wa_mm = 66.6670 # ***GET CORRECT VALUE
        ha_mm = 66.6670 # ***GET CORRECT VALUE
        wa = ha_mm / 1000.00
        ha = ha_mm / 1000.00
         #moderator
        s=numpy.zeros(6)
        s[1] = 38.60        #! *** GET CORRECT VALUE
        s[2] = 0.52260      #! *** GET CORRECT VALUE
        s[3] = 0.00     #! *** GET CORRECT VALUE
        s[4] = 0.00     #! *** GET CORRECT VALUE
        s[5] = 0.00     #! *** GET CORRECT VALUE
        th_deg = 13.0       #! *** GET CORRECT VALUE
        imod = 2            #! *** GET CORRECT VALUE
        mod_type = 'CH4'
        # sample details
        sx_mm = 20.00
        sy_mm = 19.00
        sz_mm = 50.00
        isam = 2
        gam_deg = 0.00
        ia = 0
        ix = 0
        # detector details
        idet    = 1
        dd_mm   = 25.0
        tbin_us = 0.00
        # end of mari parameters


    if instname=='maps' or instname=='map' or instname=='MAP'or instname=='MAPS':
         #For MAPS
        x0 = 10.1000
        xa = 8.1100
        x1 = 1.9000
        x2 = 6.0000
        wa_mm = 70.130
        ha_mm = 70.130
        wa = ha_mm / 1000.00
        ha = ha_mm / 1000.00

         # chopper details
         # now some moderator details
         # for 300K H2O
        s=numpy.zeros(6)
        s[1] = 38.60
        s[2] = 0.52260
        s[3] = 0.00
        s[4] = 0.00
        s[5] = 0.00
        th_deg = 32.00
        imod = 2
        mod_type = 'AP'
         # sample details
        sx_mm = 2.00
        sy_mm = 50.00
        sz_mm = 50.00
        isam = 0
        gam_deg = 0.00
        ia = 0
        ix = 0
         # detector details
        idet    = 1
        dd_mm   = 250
        tbin_us = 0.00

         #chop_par,titledata=setchoptype(instname,chop_type)
         # end of maps parameters

    if instname=='MER' or instname=='mer' or instname=='MERLIN'or instname=='merlin':
        #For HET pseudo merlin
        x0 = 10.00
        xa = 7.190
        x1 = 1.820
        x2 = 4.0420
        wa_mm = 66.6670
        ha_mm = 66.6670
        wa = ha_mm / 1000.00
        ha = ha_mm / 1000.00

        # chopper details
        # now some moderator details
        # for 300K H2O
        s=numpy.zeros(6)
        s[1] = 38.60
        s[2] = 0.52260
        s[3] = 0.00
        s[4] = 0.00
        s[5] = 0.00
        th_deg = 26.70
        imod = 2
        mod_type = 'AP'

        # sample details
        sx_mm = 2.00
        sy_mm = 40.00
        sz_mm = 40.00
        isam = 0
        gam_deg = 0.00
        ia = 0
        ix = 0

        # detector details
        idet    = 1
        dd_mm   = 250
        tbin_us = 0.00
        #chop_par,titledata=setchoptype(instname,chop_type)
        # end of HET parameters

    # Convert instrument parameters for the program (set as globals)
    pslit  = chop_par[0] / 1000.00
    dslat  = (chop_par[0] + chop_par[1]) / 1000.00
    radius = chop_par[2] / 1000.00
    rho    = chop_par[3] / 1000.00
    omega = frequency*(2*math.pi)
    tjit   = chop_par[5] * 1.0e-6


    thetam = th_deg*(math.pi/180.00)
    # function sigset set a common variable in this case to zero
    # sigset (0.0d0, 0.0d0, 0.0d0, 0.0d0)
    sx = sx_mm / 1000.00
    sy = sy_mm / 1000.00
    sz = sz_mm / 1000.00
    gam = gam_deg*math.pi/180.00

    dd = dd_mm / 1000.00
    tbin = tbin_us * 1.0e-6

    en_lo=ei-10
    en_hi=ei+10


    if kwargs.has_key('freq_dep'):
        freq=range(50,650,50)
        van_el = numpy.zeros(len(freq))
        flux = numpy.zeros(len(freq))
        err = numpy.zeros(len(freq)) #for mantid output
        for i in range(len(freq)):

            van_el[i],van,flux[i]=calc_chop(ei,freq[i]*(2*math.pi),en_lo,en_hi)
    else:

        van_el,van,flux=calc_chop(ei,omega,en_lo,en_hi)


    if kwargs.has_key('all') and kwargs.get('all')==True:
        return van_el,van,flux
    if kwargs.has_key('resolution') and kwargs.get('resolution')==True:
        if kwargs.has_key('plot') and kwargs.get('plot')==True:
        #plot data
            #recreate x range
            fac=.99
            en_lo=1
            eps_min=en_lo
            eps_max=fac*ei +(1-fac)*en_lo
            eeps=range(int(eps_min),int(eps_max),1)
            dat=list(van)
            dat.reverse()
            CreateWorkspace(OutputWorkspace="Resolution",DataX=eeps,DataY=dat,DataE=list(van*0),VerticalAxisValues="data",WorkspaceTitle='Resolution at'+str(ei)+'meV')
            plotSpectrum('Resolution',0)
            return
        return van
    if kwargs.has_key('flux') and kwargs.get('flux')==True:
        return van_el,flux
    if kwargs.has_key('freq_dep') and kwargs.get('freq_dep')==True:
        if kwargs.has_key('plot') and kwargs.get('plot')==True:
            CreateWorkspace(OutputWorkspace='Flux',DataX=freq,DataY=list(flux),DataE=list(err),VerticalAxisValues="data",WorkspaceTitle='Flux at'+str(ei)+'meV')
            plotSpectrum('Flux',0)
            CreateWorkspace(OutputWorkspace="Resolution",DataX=freq,DataY=list(van_el),DataE=list(err),VerticalAxisValues="data",WorkspaceTitle='Resolution at'+str(ei)+'meV')
            plotSpectrum('Resolution',0)
            return
        return van_el,flux
    else:
        return

def calc_chop(ei,omega,en_lo,en_hi):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    convert = 2.3548200
    # calcs flux and resoltion over a range
    # en_lo some lower range for energy for the calculation
    v_lo = max( 437.391580*math.sqrt((en_lo)), 2.00*omega/(1.00/rho + (2.00*pslit/radius**2)) )
    g_lo = max( -4.00, (2.00*radius**2/pslit)*(1.00/rho - 2.00*omega/v_lo) )
    v_hi = 437.391580*math.sqrt((en_hi))
    g_hi = min( 4.00, (2.00*radius**2/pslit)*(1.00/rho - 2.00*omega/v_hi) )
    #now get the flux and the resolution width


    if numpy.size(omega)==1:
        flux=sam_flux(ei,omega)
        v_van=van_var(ei,omega)
        #do the conversions for the resolutions
        flux = numpy.real(100e15 * flux)
        van_el = numpy.real( convert * 8.747832e-4 * math.sqrt(ei**3) * ( math.sqrt(v_van + (tbin**2/12.00)) * 1.0e6 ) / x2 )

        #if length(ei) > 1
        #titledata2=['Resoluion and flux for the ' titledata ' at ' num2str((omega/(2*pi))) ' Hz']
        #subplot(2,1,1), title(titledata2)
        #subplot(2,1,1), xlabel('Ei [mev]'), ylabel('flux [ncm.^{-2}s.^{-1}/100uA]')
        #subplot(2,1,2), xlabel('Ei [mev]'), ylabel('Vanadium width [meV]')
        #else
        #For a single ei always display the resolution
        print 'Flux at sample position at ',ei,'meV',int(omega/(2*math.pi)), 'Hz =',int(flux),'ns^-1'
        # calc resolution as a function of energy trans
        print 'Resolution of elastic line at ',int(omega/(2*math.pi)), 'Hz = ',van_el, 'meV = ',(van_el/ei)*100, '%'
        fac=.99
        en_lo=1
        eps_min=en_lo
        eps_max=fac*ei +(1-fac)*en_lo
        eeps=range(int(eps_min),int(eps_max),1)
        van=numpy.zeros(numpy.size(eeps))
        for i in range(numpy.size(eeps)):
            etrans=ei-eeps[i]
            v_van=van_var(ei,omega,etrans)
            van[i] = numpy.real( convert * 8.747832e-4 * math.sqrt((ei-etrans)**3) * ( math.sqrt(v_van + (tbin**2/12.00)) * 1.0e6 ) / x2 )

        return van_el,van,flux
        #plot(eeps,fliplr(van),'o-')
        #output=[eeps fliplr(van)']
        #titledata3=['Resoluion ' titledata ' at ' num2str((omega/(2*pi))) ' Hz']
        #title(titledata3)
        #xlabel('Energy Transfer [mev]'), ylabel('Resolution [meV]')


    #subplot(2,1,1), plot((omega/(2*pi)),fluxa,'o-')
    #subplot(2,1,2), plot((omega/(2*pi)),vana,'o-')
    #titledata2=['Resoluion and flux for the ' titledata ' at ' num2str(ei) ' meV']
    #subplot(2,1,1), title(titledata2)
    #subplot(2,1,1), xlabel('Frequency [Hz]'), ylabel('flux [ncm.^{-2}s.^{-1}/100uA]')
    #subplot(2,1,2), xlabel('Frequency [Hz]'), ylabel('Vanadium width [meV]')

def van_var(*args):
    """
    calcuate vanadium widths at ei and etrans
    vanvar(ei,omega,etrans):
    """
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata

    if len(args) == 3:
        ei =args[0]
        omega=args[1]
        eps=args[2]

    else:
        ei =args[0]
        omega=args[1]
        eps=0



    wi = 0.69468875*math.sqrt(ei)
    wf = 0.69468875*math.sqrt(ei-eps)

#
# !  get a load of widths:
# !  ---------------------
# !  moderator:
    if imod == 0:
        tsqmod=tchi(s[1]/1000.00, ei)
    elif imod == 1:
        tsqmod=tikeda(s[1], s[2], s[3], s[4], s[5], ei)
    elif imod == 2:
        tsqmod=tchi_2((s[1]/1000.00), (s[2]/1000.00), ei)

#!  chopper:
    tsqchp,ifail=tchop(omega, ei)
    ifail
    if ifail <> 0:
        tsqchp = 0.0


#!  chopper jitter:
    tsqjit = tjit**2


    v_x, v_y, v_z=sam0(6)
    atten=1.00
    dx=0.00
    dy=0.00
    dz=0.00
    v_xy=0.00

# !  detector:
    deld, sigd, sigdz, sigdd, effic=detect2(1.00, 1.00, wf, 6)
    v_dd=sigdd**2

# !  now calculate the inelastic vanadium width:
# !  -------------------------------------------
    phi=10
    v_van=van_calc(tsqmod, tsqchp, tsqjit, v_x, v_y, v_xy, v_dd, ei, eps, phi,omega)
    return v_van


#moderator functions

def tikeda(S1,S2,B1,B2,EMOD,ei):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    SIG=math.sqrt( (S1*S1) + ((S2*S2*81.8048)/ei) )
    A = 4.37392e-4 * SIG * math.sqrt(ei)
    for j in range(len(ei)):
        if ei[j] > 130.0:
            B[j]=B2
        else:
            B[j]=B1


        R=math.math.exp(-ei/EMOD)
        tausqr[j]=(3.0/(A*A)) + (R*(2.0-R))/(B[j]*B[j])

    # variance currently in mms**2. Convert to sec**2

    return tausqr*1.0e-12

def tchi(DELTA,ei):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    VEL=437.392*math.sqrt(ei)
    tausqr=( (DELTA/1.96)/ VEL )**2
    return tausqr

def tchi_2(DELTA_0,DELTA_G,ei):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    VEL=437.392*math.sqrt(ei)
    tausqr=( ( (DELTA_0+DELTA_G*math.sqrt(ei))/1.96) / VEL )**2
    return tausqr
# end of moderator functions

def tchop(omega,ei):
#chopper
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    p=pslit
    R=radius
    rho=rho
    w=omega
    ei=ei

    if p == 0.00 and R == 0.00 and rho == 0.00:
        ierr=1
        tausqr = 0.00


# !  Calculate parameter gam:
# ! --------------------------
    veloc=437.3920*math.sqrt(ei)
    gammm=( 2.00*(R**2)/p ) * abs(1.00/rho - 2.00*w/veloc)

#!  Find regime and calculate variance:
#! -------------------------------------
    #for j in range(len(ei)):
    groot=0
    if gammm >= 4.00:
        ierr=1
        tausqr=0.00
    else:
        ierr=0
    if gammm <= 1.00:
        gsqr=(1.00-(gammm**2)**2 /10.00) / (1.00-(gammm**2)/6.00)
    else:
        groot=math.sqrt(gammm)
        gsqr=0.60*gammm*((groot-2.00)**2)*(groot+8.00)/(groot+4.00)

    tausqr=( (p/(2.00*R*w))**2/ 6.00) * gsqr
    return tausqr,ierr


def sam0(iout):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    varx=1
    vary=1
    varz=1
    return varx, vary, varz


def detect2(wd,hd,wf,iout):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    unity=1.0
    ierr=0
# seems this is the only useable function
# !  He cylindrical detectors binned together
# ! -----------------------------------------
    rad=dd/2.0
    atms=10.0
    t2rad=0.063

#don't call this function at the mo and approximate some values for it
#[effic,delta,ddsqr,v_dd,v_d]=detect_he(wf,rad,atms,t2rad)
    effic=.5
    delta=0
    ddsqr=0.25
    v_dd=0.01
    v_d=0.01
    sigd =wd/math.sqrt(12.0)
    sigdz=hd/math.sqrt(12.0)
    sigdd=math.sqrt(v_dd)
    return delta,sigd,sigdz,sigdd,effic

def van_calc(v_mod, v_ch, v_jit, v_x, v_y, v_xy, v_dd,ei, eps, phi,omega):
#[v_van,v_van_m,v_van_ch,v_van_jit,v_van_ya,v_van_x,v_van_y,v_van_xy,v_van_dd]
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    veli=437.39160*math.sqrt( ei )
    velf=437.39160*math.sqrt( ei-eps )
    rat=(veli/velf)**3


    tanthm=math.tan(thetam)

    am   = -(x1+rat*x2)/x0
    ach  = (1.00 + (x1+rat*x2)/x0)
    g1 = (1.00 - (omega*(x0+x1)*tanthm/veli))
    g2 = (1.00 - (omega*(x0-xa)*tanthm/veli) )
    f1 =  1.00 + ((x1/x0)*g1)
    f2 =  1.00 + ((x1/x0)*g2)
    gg1 = g1 / ( omega*(xa+x1) )
    gg2 = g2 / ( omega*(xa+x1) )
    ff1 = f1 / ( omega*(xa+x1) )
    ff2 = f2 / ( omega*(xa+x1) )
    aa = ( (math.cos(gam)/veli) - (math.cos(gam-phi)/velf) ) - (ff2*math.sin(gam))
    bb = ((-math.sin(gam)/veli) + (math.sin(gam-phi)/velf) ) - (ff2*math.cos(gam))
    aya  = ff1 + ((rat*x2/x0)*gg1)
    ax   = aa  - ((rat*x2/x0)*gg2*math.sin(gam))
    ay   = bb  - ((rat*x2/x0)*gg2*math.cos(gam))
    a_dd  = 1.00/velf

    v_van_m  = am**2  * v_mod
    v_van_ch = ach**2 * v_ch
    v_van_jit= ach**2 * v_jit
    v_van_ya = aya**2 * (wa**2/12.00)
    v_van_x  = ax**2  * v_x
    v_van_y  = ay**2  * v_y
    v_van_xy = ax*ay  * v_xy
    v_van_dd = a_dd**2* v_dd

    v_van = (v_van_m + v_van_ch + v_van_jit + v_van_ya)
    return v_van


def sam_flux(ei,omega):
    '''# Calculates the flux at the sample position for HET or MARI in  n / cm**2 . uA.s for the U target
    # (all distances in m, angles in rad, omega rad/s, ei in meV)
    '''
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    flux=[]
    flux1=flux_calc(ei)
    area=achop(ei,omega)
    #for j in range(len(ei)):
    flux = 84403.060*ei*(flux1/math.cos(thetam))*(area/dslat)*(wa*ha)/(x0*(x1+xa)**2)

    return flux

def flux_calc(ei):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    conv1=3.615
    conv2=9.104157e-12
    conv=conv1*conv2
    en_ev=ei/1000.00
    ch_mod=mod_type
    phi0=flux_norm(ch_mod)
    phifun=flux_fun( en_ev, ch_mod)

    flux=(conv*( phi0*math.cos(thetam) )*(math.sqrt(en_ev)*phifun))/1
    return flux

def flux_norm(ch_mod):
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    if ch_mod =='A':
        phi0=1.00
    if ch_mod == 'AP':
        phi0=2.80
    if ch_mod =='H2':
        phi0=1.80
    if ch_mod == 'CH4':
        phi0=2.60

    return phi0

def flux_fun( en_ev, ch_mod):
    '''
    % !
    % !  Calculates the energy dependence of the flux (see Perring et al
    % ! RAL-85-029)
    % !
    % !  Entry
    % !     en_ev  : eV
    % !     ch_mod : 'A'  'AP'  'CH4'  'H2' 'TEST'
    % !
    % !  Exit
    % !   phifun   : the functional dependace with energy
    '''
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    if ch_mod =='A':
        ijoin=0
        rj=0.00
        t=0.00
        a=0.00
        w1=0.00
        w2=0.00
        w3=0.00
        w4=0.00
        w5=0.00
        ierr=1
    if ch_mod =='AP':
        ijoin=0
        rj=2.250
        t=0.0320
        a=0.950
        w1=120.00
        w2=10.00
        w3=0.00
        w4=0.00
        w5=0.00
    if ch_mod =='H2':
        ijoin=1
        rj=2.350
        t=0.00210
        a=0.950
        w1=15.50
        w2=3.10
        w3=11.00
        w4=0.2540
        w5=0.02750
    if ch_mod =='CH4':
        ijoin=0
        rj=2.10
        t=0.0110
        a=0.920
        w1=55.0
        w2=7.00
        w3=0.00
        w4=0.00
        w5=0.00

#
#
# ! Calculation:
# ! ------------
    phi_max=rj*(en_ev/(t**2))*math.exp(-en_ev/t)
    phi_epi=1.00/(en_ev)**a

    math.expon=math.exp(-w1/math.sqrt(1000.00*en_ev)+w2)
    delt1=math.expon/ (  1.00 + math.expon  )

    if ijoin == 1:
        math.expon=math.exp( (w4 - 1.00/math.sqrt(1000.00*en_ev) )/w5 )
        delt2=1.00 + w3/( 1.00 + math.expon )
    else:
        delt2=1.00


    phifun=phi_max + delt1*delt2*phi_epi
    return phifun
def achop(ei,omega):
    '''
    # !
    # !   Calculates the integral of the chopper transmission function
    # !  P(h,t) over time and distance for any energy. The answer is in m.S
    # !   New version 15/1/90
    # !
    # !    p       slit thickness (m)                              R*8
    # !    R       slit package diameter (m)                       R*8
    # !    rho     slit radius of curvature (m)                    R*8
    # !    w       angular frequency of rotor (rad/sec)            R*8
    # !    ei      energy the rotor has been phased for (meV)      R*8
    # !    area    intergral                                       R*8
    # !    ierr    error indicator                                 integer
    # !               =0  no problems
    # !               =1  if no transmission  AREA set to zero
    '''
    global x0, xa, x1, x2, wa_mm, ha_mm, wa, ha, pslit
    global dslat, radius, rho, tjit, mod_type, s, thetam, mod_type
    global imod, sx, sy, sz, isam, gam, ia, ix, idet, dd, tbin, titledata
    dd=dslat
    p1=pslit
    R1=radius
    rho1=rho
    w1=omega
    ei=ei

    vela=437.3920*math.sqrt(ei)
    gamm=( 2.00*(R1**2)/p1 ) * abs(1.00/rho1 - 2.00*w1/vela)

# !  Find regime and calculate variance:
# ! -------------------------------------

    #for j in range(numpy.size(ei)):
    groot=0
    if gamm >= 4.00:
        f1=0
        print 'no transmission at ', ei, 'meV at ',omega/(2*math.pi), 'Hz'
    else:
        ierr=0
    if gamm <= 1.00:
        f1=1.00-(gamm**2)/6.00
    else:
        groot=math.sqrt(gamm)
        f1=groot*((groot-2.00)**2)*(groot+4.00)/6.00



    area=( (p1**2)/(2.00*R1*w1) ) * f1
    return area

def frange(limit1, limit2 = None, increment = 1.):
    """
  Range function that accepts floats (and integers).

  Usage:
  frange(-2, 2, 0.1)
  frange(10)
  frange(10, increment = 0.5)

  The returned value is an iterator.  Use list(frange) for a list.
  """

    if limit2 is None:
        limit2, limit1 = limit1, 0.
    else:
        limit1 = float(limit1)

    count = int(math.ceil(limit2 - limit1)/increment)
    return (limit1 + n*increment for n in range(count))
