import numpy as np
from input import INPUT
from start import START
from back import BACK
from maxent import MAXENT
from deadfit import DEADFIT
from modbak import MODBAK
from modamp import MODAMP
from modab import MODAB
#from output import OUTPUT
from outspec import OUTSPEC

# translation of multimaxalpha.for
"""
cc this is multimax.for - jan 96 multiple numor reads and currant bun 
c  found the asymmetry bug 9/1/96: asymmetry with free phases was wrong
c  link with multibunrd
C
C**********************************************************************
C          RAL VERSION OF MAXENT - MAIN PROGRAM : reads runs itself
C**********************************************************************
C      
c      PROGRAM multimax
      subroutine multimax


      use maxdef
     	use dflib
	use dflogm


      REAL DATUM(68000),SIGMA(68000),F(68000),BASE(68000),
     +CONVOLR(8192),CONVOLI(8192),HISTS(64)
      INTEGER P,GROUP(64)
      CHARACTER*1 ANS,AN,FITDEAD,FIXPHASE,FITAMP,firstgo
      COMMON/FILE/NAME
      COMMON/DETECT/A,B,E,c,d
      REAL A(64),B(64),c(64),d(64),E(8192)
      common/sense/phi(64),TAUD(64)
      REAL TAUD
      REAL CORR(68000),DATT(68000)
	character*2046 str
      common/fac/factor,facdef,facfake,ratio
      common/channels/itzero,ipulse1,ipulse2,i1stgood,itotal
      common/points/npts,ngroups,nhists
      common/pulses/npulse,DEF
      common/heritage/iter
      COMMON/FLAGS/FITDEAD,FIXPHASE,FITAMP
      COMMON/RUNDATA/RES,FRAMES,FNORM,IRUNNO,HISTS
      COMMON/MISSCHANNELS/MM
      common/savetime/ngo,i2pwr
      COMMON/ANS/ANS,firstgo
	common/MaxPage/n,f
     	type (qwinfo)  qw  

      P=NPTS*NGROUPS
      CALL INPUT(NHISTS,NGROUPS,NPTS,P,DATUM,SIGMA,CORR,DATT)
	write(99,*) "back from input"
      CALL START(NGROUPS,NPTS,P)
      CALL BACK(HISTS,NGROUPS,NPTS,P,DATUM,SIGMA,CORR)
      ngo=-1
      DO 999 J=1,10
      ngo=ngo+1
      write(str,'('' CYCLE NUMBER='',i2)')ngo
	call module_print(TRIM(str))

	

        write(str,345)
345     format(' iter  test:<.02?   entropy    chitarget     chisq',
     +   '      freqsum ')
	call module_print(TRIM(str))
1     CALL MAXENT(NGROUPS,NPTS,P,DATUM,SIGMA,def,BASE,10,.FALSE.)
      IF (fitdead.eq.'Y') THEN
          CALL DEADFIT(NGROUPS,NPTS,P,DATUM,SIGMA
     +                  ,CORR,DATT)
      ELSE  
          CALL MODBAK(HISTS,NGROUPS,NPTS,P,DATUM,SIGMA)
      ENDIF
      IF(fixphase.EQ.'Y') THEN
          CALL MODAMP(HISTS,NGROUPS,NPTS,P,DATUM,SIGMA)
      ELSE
          CALL MODAB(HISTS,NGROUPS,NPTS,P,DATUM,SIGMA)
      ENDIF
c      if( (iter.eq.2).and.(ngo.ge.3) ) then
c        write(99,*) ' Getting there in one iteration, so stopping...'
c        goto 997
c      endif
!      if( (iter.eq.3).and.(ngo.ge.5) ) then
!        write(99,*) ' Pretty close, but chi**2 a bit tight: stop? [Y]'
!        read(5,110) an
!        IF(AN.NE.'N'.AND.AN.NE.'n') GOTO 997
!      endif        
999   CONTINUE
997   CALL OUTPUT(P)
      CALL OUTSPEC(NGROUPS,NPTS,N,P,DATUM,F,SIGMA,datt)
!      write(99,*) ' Again? [y]'
!      read(5,110) AN
110    FORMAT(A1)
!      IF(AN.NE.'N'.AND.AN.NE.'n') GOTO 111    
!      STOP      
      return
      END
"""
def MULTIMAX(POINTS_nhists,POINTS_ngroups,POINTS_npts,CHANNELS_itzero,CHANNELS_i1stgood,CHANNELS_itotal,RUNDATA_res,RUNDATA_frames,
						GROUPING_group,DATALL_rdata,FAC_factor,SENSE_taud,MAXPAGE_n,filePHASE,PULSES_def,PULSES_npulse,FLAGS_fitdead,FLAGS_fixphase,SAVETIME_i2,
						OuterIter,InnerIter,mylog,prog,phaseconvWS,TZERO_fine):
	#
	base=np.zeros([MAXPAGE_n])
	(datum,sigma,corr,datt,MISSCHANNELS_mm,RUNDATA_fnorm,RUNDATA_hists,FAC_facfake,FAC_ratio)=INPUT(
					POINTS_nhists,POINTS_ngroups,POINTS_npts,CHANNELS_itzero,CHANNELS_i1stgood,CHANNELS_itotal,
					RUNDATA_res,RUNDATA_frames,GROUPING_group,DATALL_rdata,FAC_factor,SENSE_taud,mylog)
					
	(DETECT_e,PULSESHAPE_convol)=START(
					POINTS_npts,PULSES_npulse,RUNDATA_res,MAXPAGE_n,TZERO_fine,mylog)
		
	(datum,DETECT_a,DETECT_b,DETECT_d,FASE_phase)=BACK(RUNDATA_hists,datum,sigma,DETECT_e,filePHASE,mylog)
	SAVETIME_ngo=-1
	MAXPAGE_f=None
	for j in range(OuterIter): # outer "alpha chop" iterations?
		SAVETIME_ngo=SAVETIME_ngo+1
		mylog.information("CYCLE NUMBER="+str(SAVETIME_ngo))
		(sigma,base,HERITAGE_iter,MAXPAGE_f,FAC_factor,FAC_facfake)=MAXENT(
						datum,sigma,PULSES_def,base,InnerIter,False,
						SAVETIME_ngo,MAXPAGE_n,MAXPAGE_f,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e,FAC_factor,FAC_facfake,SAVETIME_i2,mylog,prog)

		if(FLAGS_fitdead):
			(datum,corr,DETECT_c,DETECT_d,SENSE_taud)=DEADFIT(
							datum,sigma,datt,DETECT_a,DETECT_b,DETECT_d,DETECT_e,RUNDATA_res,RUNDATA_frames,RUNDATA_fnorm,RUNDATA_hists,
							MAXPAGE_n,MAXPAGE_f,PULSESHAPE_convol,SAVETIME_i2,mylog)
		else:
			(DETECT_c,DETECT_d)=MODBAK(RUNDATA_hists,datum,sigma,DETECT_a,DETECT_b,DETECT_e,DETECT_d,MAXPAGE_f,PULSESHAPE_convol,SAVETIME_i2,mylog)
		
		if(FLAGS_fixphase):
			(SENSE_phi,DETECT_a,DETECT_b,AMPS_amp)=MODAMP(RUNDATA_hists,datum,sigma,MISSCHANNELS_mm,FASE_phase,MAXPAGE_f,PULSESHAPE_convol,DETECT_e,SAVETIME_i2,mylog)
		else:
			(SENSE_phi,DETECT_a,DETECT_b,AMPS_amp)=MODAB(RUNDATA_hists,datum,sigma,MISSCHANNELS_mm,MAXPAGE_f,PULSESHAPE_convol,DETECT_e,SAVETIME_i2,mylog)
		# outout per-iteration debug info
		if(phaseconvWS):
			for k in range(POINTS_ngroups):
				phaseconvWS.dataX(k)[j+1]=(j+1)*1.0
				phaseconvWS.dataY(k)[j+1]=SENSE_phi[k]
		prog.report((j+1)*InnerIter,"") # finished outer loop, jump progress bar
	
	# OUTPUT(POINTS_npts,RUNDATA_res,RUNDATA_irunno,MAXPAGE_n,MAXPAGE_f)
	(OUTSPEC_test,OUTSPEC_guess)=OUTSPEC(datum,MAXPAGE_f,sigma,datt,CHANNELS_itzero,CHANNELS_itotal,
							PULSESHAPE_convol,FAC_ratio,DETECT_a,DETECT_b,DETECT_d,DETECT_e,SAVETIME_i2,RUNDATA_fnorm,mylog)
	
	return (MISSCHANNELS_mm,RUNDATA_fnorm,RUNDATA_hists,MAXPAGE_f,FAC_factor,FAC_facfake,FAC_ratio,
				DETECT_a,DETECT_b,DETECT_c,DETECT_d,DETECT_e,PULSESHAPE_convol,SENSE_taud,FASE_phase,SAVETIME_ngo,
				AMPS_amp,SENSE_phi,OUTSPEC_test,OUTSPEC_guess)
	