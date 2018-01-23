import numpy as np
from zft import ZFT

# translation of deadfit.for
"""
      SUBROUTINE DEADFIT(NGROUPS,NPTS,P,DATUM,SIGMA,
     +CORR,DATT)
      INTEGER P
      REAL DATUM(NPTS,NGROUPS),SIGMA(NPTS,NGROUPS),F(68000)
      REAL ZR(8192),ZI(8192),TAUD,CORR(NPTS,NGROUPS)
      REAL A(64),B(64),c(64),d(64),E(8192)
      REAL DATT(NPTS,NGROUPS),HISTS(64)
	character*2046 str
      common/savetime/ngo,i2pwr
      common/fac/factor,facdef,facfake,ratio
      COMMON/DETECT/A,B,E,c,d
      COMMON/RUNDATA/RES,FRAMES,FNORM,IRUNNO,HISTS
      COMMON/MISSCHANNELS/MM
      common/sense/phi(64),TAUD(64)
	common/MaxPage/n,f
      CALL ZFT(P,N,F,ZR,ZI)
      DO 3 J=1,NGROUPS
      IF(HISTS(J).EQ.0)GOTO 3
      Ax=0.
      Bx=0.
      Cx=0.
      Dx=0.
      Ex=0.
c      write(99,*) ' k,e,datt,diff,wiggle,sigma**2'
      DO 4 K=1,npts
      if (sigma(k,j).gt.1.e10) goto 4
      DAT=DATT(K,J)
      sig=sigma(k,j)*sigma(k,j)
      wiggle=(A(J)*ZR(K)+B(J)*ZI(K))
      DIFF=DAT-wiggle
C      if((k.le.2048).and.(j.eq.1)) then
C        write(99,777) k,e(k),dat,diff,wiggle,sig
C777     format(1x,i4,1x,f5.3,1x,f8.2,1x,f8.2,1x,f8.2,1x,f8.2)
C      endif
c      sig=e(k)
      Ax=Ax+(E(K)*E(K))/SIG
      Bx=Bx+(E(K)*DIFF)/SIG
      Cx=Cx+(E(K)*DAT*DAT)/SIG
      Dx=Dx+(DAT**4)/SIG
      Ex=Ex+(DIFF*DAT*DAT)/SIG
4     continue
c      write(99,*) ' AX TO EX , tau, scale, taud(j)'
      tau=(Bx*Cx-Ax*Ex)/(Ax*Dx-Cx*Cx)
      scale=(Bx/Ax)+tau*(Cx/Ax)
      taud(J)=tau*res*hists(J)*frames*fnorm
c      write(99,*) AX,BX,CX,DX,EX,tau,scale,taud(j)
      c(j)=scale-d(j)
      d(j)=scale
c      IF (TAU.LT.0) THEN
c        write(99,*) ' whoops '
c      ENDIF
      DO 5 K=1,npts
      if(sigma(k,j).gt.1.e3) goto 5
      CORR(K,J)=(DATT(K,J)*DATT(K,J))*TAU
      DATUM(K,J)=DATT(K,J)+CORR(K,J)-D(J)*E(K)
5     continue
3     CONTINUE
c      write(99,*) ' ORIGINAL DATA AND EXPONENTIAL: k=50,j=1,4...'
c      write(99,*) (DATT(50,J),J=1,4)
c      write(99,*) (D(J)*E(50),J=1,4)
c      write(99,*) ' DATUM AND CORRECTION... '
c      write(99,*) (DATUM(50,J),J=1,4)
c      write(99,*) (CORR(50,J),J=1,4)
c      write(99,*) ' WIGGLE...'
c      write(99,*) (A(J)*ZR(50)+B(J)*ZI(50),J=1,4)
c      write(99,*) ' amplitudes of exponentials:'
c      write(99,*) (d(J),J=1,NGROUPS)
	call module_print(" amplitudes of exponentials:")
	write(str,*) (D(J),J=1,NGROUPS)
	call module_print(TRIM(str))

c      write(99,*) ' changes this cycle:'
c      write(99,*) (c(J),J=1,NGROUPS)
 	call module_print(" changes this cycle:")
	write(str,*) (c(J),J=1,NGROUPS)
	call module_print(TRIM(str))

c     write(99,*) ' DEADTIMES:'
c      write(99,*) (TAUD(J),J=1,NGROUPS)
 	call module_print(" DEADTIMES:")
	write(str,*) (TAUD(J),J=1,NGROUPS)
	call module_print(TRIM(str))
 
      RETURN
      END
"""
def DEADFIT(datum,sigma,datt,DETECT_a,DETECT_b,DETECT_d,DETECT_e,RUNDATA_res,RUNDATA_frames,RUNDATA_fnorm,RUNDATA_hists,
					MAXPAGE_n,MAXPAGE_f,PULSESHAPE_convol,SAVETIME_I2,mylog):
	(npts,ngroups)=datt.shape
	#dat=np.array(datt)
	#corr=np.zeros([npts,ngroups])
	#DETECT_c=np.zeros([ngroups])
	#DETECT_d=np.zeros([ngroups])
	#SENSE_taud=np.zeros([ngroups])
	zr,zi=ZFT(MAXPAGE_f,PULSESHAPE_convol,DETECT_e,SAVETIME_I2)
	# new numpy array code
	isig=sigma**-2
	wiggle=np.outer(zr,DETECT_a)+np.outer(zi,DETECT_b)
	diff=datt-wiggle
	Ax=np.dot(DETECT_e**2,isig)
	Bx=np.dot(DETECT_e,isig*diff)
	Cx=np.dot(DETECT_e,isig*datt**2)
	Dx=np.sum(datt**4*isig,axis=0)
	Ex=np.sum(diff*datt**2*isig,axis=0)
	tau=(Bx*Cx-Ax*Ex)/(Ax*Dx-Cx*Cx)
	scale=(Bx/Ax)+tau*(Cx/Ax)
	SENSE_taud=tau*RUNDATA_res*RUNDATA_hists*RUNDATA_frames*RUNDATA_fnorm
	DETECT_c=scale-DETECT_d
	DETECT_d=scale
	corr=datt**2*tau[np.newaxis,:]
	datum=np.where(sigma<=1.E3, datt+corr-np.outer(DETECT_e,DETECT_d), datum)
	# orig loops
	#for j in range(ngroups):
	#	if(hists[j]!=0):
	#		Ax=0.
	#		Bx=0.
	#		Cx=0.
	#		Dx=0.
	#		Ex=0.
	#		for k in range(npts):
	#			if(sigma[k,j]<=1.E10):
	#				dat=datt[k,j]
	#				sig=sigma[k,j]**2
	#				wiggle=(DETECT_a[j]*zr[k]+DETECT_b*zi[k])
	#				diff=dat-wiggle
	#				Ax=Ax+(DETECT_e[k]**2)/sig
	#				Bx=Bx+(DETECT_e[k]*diff)/sig
	#				Cx=Cx+(DETECT_e[k]*dat*dat)/sig
	#				Dx=Dx+(dat**4)/sig
	#				Ex=Ex+(diff*dat**2)/sig
	#		tau=(Bx*Cx-Ax*Ex)/(Ax*Dx-Cx*Cx)
	#		scale=(Bx/Ax)+tau*(Cx/Ax)
	#		SENSE_taud[j]=tau*res*hists[j]*frames*fnorm
	#		DETECT_c[j]=scale-DETECT_d[j]
	#		DETECT_d[j]=scale
	#		for k in range(npts):
	#			if(sigma[k,j]<=1.E3):
	#				corr[k,j]=(datt[k,j]**2)*tau
	#				datum[k,j]=datt[k,j]+corr[k,j]-DETECT_d[j]*DETECT_e[k]
	mylog.debug("amplitudes of exponentials:"+str(DETECT_d))
	mylog.debug("changes this cycle:"+str(DETECT_c))
	mylog.debug("DEADTIMES:"+str(SENSE_taud))
	return datum,corr,DETECT_c,DETECT_d,SENSE_taud

