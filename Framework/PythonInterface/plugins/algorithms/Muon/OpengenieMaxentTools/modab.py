import numpy as np
import math
from zft import ZFT

# translation of modab.for
"""
      SUBROUTINE MODAB(HISTS,NGROUPS,NPTS,P,DATUM,SIGMA)
      INTEGER P
      REAL DATUM(NPTS,NGROUPS),F(68000),SIGMA(NPTS,NGROUPS)
      REAL AMP(64),HISTS(NGROUPS)
      REAL ZR(8192),ZI(8192)
	character*2046 str
      common/savetime/ngo,i2pwr
      common/sense/phi(64)
      COMMON/MISSCHANNELS/MM
      COMMON/DETECT/A,B,E,c,d
      common/amps/amp
      CHARACTER AN
      REAL A(64),B(64),c(64),d(64),E(8192)
	common/MaxPage/n,f
      CALL ZFT(P,N,F,ZR,ZI)
      DO 3 J=1,NGROUPS
      IF(HISTS(J).EQ.0)GOTO 3
      S11=0.
      S12=0.
      S22=0.
      SA=0.
      SB=0.
      DO 4  K=1,npts
      X=ZR(K)/SIGMA(K,J)
      Y=ZI(K)/SIGMA(K,J)
      S11=S11+X*X
      S22=S22+Y*Y
      S12=S12+X*Y
      SA=SA+DATUM(K,J)*X/SIGMA(K,J)
4     SB=SB+DATUM(K,J)*Y/SIGMA(K,J)
      A(J)=(SA*S22-SB*S12)/(S11*S22-S12*S12)
      B(J)=(SB*S11-SA*S12)/(S11*S22-S12*S12)
      AMP(J)=SQRT(A(J)*A(J)+B(J)*B(J))
      PHI(J)=57.296*ATAN2(B(J),A(J))
3     CONTINUE
      S=0.
      DO 5 J=1,NGROUPS
5     S=S+AMP(J)
      S=S/FLOAT(NGROUPS-MM)
c      write(99,*) ' s is... ',s
c      write(99,7)
c7     FORMAT(1X,'AMPLITUDES  ')
c      write(99,*) (AMP(J)/S,J=1,NGROUPS)
      call module_print(" AMPLITUDES")
	write(str,*) (AMP(J)/S,J=1,NGROUPS)
	call module_print(TRIM(str))

c      write(99,8) 
c8     FORMAT(1X,'PHASES =  ')
c      write(99,*)(PHI(J),J=1,NGROUPS)
      call module_print(" PHASES =  ")
	write(str,*) (PHI(J),J=1,NGROUPS)
	call module_print(TRIM(str))

      DO 6 J=1,NGROUPS
      amp(j)=amp(j)/s
      A(J)=A(J)/S
6     B(J)=B(J)/S
      RETURN
      END
"""
# P unused apart from passing to ZFT which doesn't use it
# params ngroups, npts inferred from size of datum

def MODAB(hists,datum,sigma,MISSCHANNELS_mm,MAXPAGE_f,PULSESHAPE_convol,DETECT_e,SAVETIME_I2,mylog):
	npts,ngroups=datum.shape
	zr,zi=ZFT(MAXPAGE_f,PULSESHAPE_convol,DETECT_e,SAVETIME_I2)
	DETECT_a=np.zeros([ngroups])
	DETECT_b=np.zeros([ngroups])
	#SENSE_phi=np.zeros([ngroups])
	#AMPS_amp=np.zeros([ngroups])
	for j in range(ngroups):
		if(hists[j]!=0):
			x=zr/sigma[:,j]
			y=zi/sigma[:,j]
			s11=np.sum(x**2)
			s22=np.sum(y**2)
			s12=np.sum(x*y)
			sa=np.sum(datum[:,j]*x/sigma[:,j])
			sb=np.sum(datum[:,j]*y/sigma[:,j])
			DETECT_a[j]=(sa*s22-sb*s12)/(s11*s22-s12*s12)
			DETECT_b[j]=(sb*s11-sa*s12)/(s11*s22-s12*s12)
	AMPS_amp=np.sqrt(DETECT_a**2+DETECT_b**2)
	SENSE_phi=np.arctan2(DETECT_b,DETECT_a) # *180.0/math.pi
	s=np.sum(AMPS_amp)/float(ngroups-MISSCHANNELS_mm)
	mylog.notice("AMPLITUDES"+str(AMPS_amp/s))
	mylog.notice("PHASES ="+str(SENSE_phi))
	AMPS_amp=AMPS_amp/s
	DETECT_a=DETECT_a/s
	DETECT_b=DETECT_b/s
	return (SENSE_phi,DETECT_a,DETECT_b,AMPS_amp)
	