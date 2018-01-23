import numpy as np
from zft import ZFT

# translation of modbak.for
"""
      SUBROUTINE MODBAK(HISTS,NGROUPS,NPTS,P,DATUM,SIGMA)
      INTEGER P
      REAL DATUM(NPTS,NGROUPS),SIGMA(NPTS,NGROUPS),F(68000)
      REAL ZR(8192),ZI(8192),HISTS(NGROUPS)
      REAL A(64),B(64),c(64),d(64),E(8192)
	character*255 str
      common/savetime/ngo,i2pwr
      common/fac/factor,facdef,facfake,ratio
      COMMON/DETECT/A,B,E,c,d
      COMMON/MISSCHANNELS/MM
	common/MaxPage/n,f
      CALL ZFT(P,N,F,ZR,ZI)
      DO 3 J=1,NGROUPS
      IF(HISTS(J).EQ.0)GOTO 3
      S=0.
      T=0.
      ssum=0.
      nss=0
      DO 4 K=1,npts
      if (sigma(k,j).gt.1.e3) goto 4
      diff=DATUM(K,J)-(A(J)*ZR(K)+B(J)*ZI(K))
      S=S+diff*E(K)/SIGMA(K,J)**2
      T=T+(E(K)/SIGMA(K,J))**2
4     continue
      SCALE=S/T
      c(j)=scale
      d(j)=d(j)+scale
      DO 5 K=1,npts
      if(sigma(k,j).gt.1.e3) goto 5
      DATUM(K,J)=DATUM(K,J)-SCALE*E(K)
5     continue
3     CONTINUE
      write(str,*) ' exponentials:'
		call module_print(TRIM(str))
      write(str,*) (d(J),J=1,NGROUPS)
		call module_print(TRIM(str))
      write(str,*) ' changes this cycle:'
		call module_print(TRIM(str))
      write(str,*) (c(J),J=1,NGROUPS)
		call module_print(TRIM(str))
      RETURN
      END
"""
#
def MODBAK(hists,datum,sigma,DETECT_a,DETECT_b,DETECT_e,DETECT_d,MAXPAGE_f,PULSESHAPE_convol,SAVETIME_I2,mylog):
	npts,ngroups=datum.shape
	DETECT_c=np.zeros([ngroups])
	zr,zi=ZFT(MAXPAGE_f,PULSESHAPE_convol,DETECT_e,SAVETIME_I2)
	for j in range(ngroups):
		if(hists[j]!=0):
			isigsq=np.where(sigma[:,j]>1.E3,0.0,sigma[:,j]**-2)
			diff=datum[:,j]-(DETECT_a[j]*zr+DETECT_b[j]*zi)
			s=np.sum(diff*DETECT_e*isigsq)
			t=np.sum(isigsq*DETECT_e)
			scale=s/t
			DETECT_c[j]=scale
			DETECT_d[j]=DETECT_d[j]+scale
			datum[:,j]=datum[:,j]-scale*np.where(sigma[:,j]>1.E3,0.0,DETECT_e)
	mylog.debug("exponentials:"+str(DETECT_d))
	mylog.debug("changes this cycle:"+str(DETECT_c))
	return DETECT_c,DETECT_d
	
