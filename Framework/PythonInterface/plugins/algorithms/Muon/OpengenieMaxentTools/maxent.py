import numpy as np
import math
from opus import OPUS
from tropus import TROPUS
from project import PROJECT
from move import MOVE

# translated from MAXENT.for
"""
      SUBROUTINE MAXENT(NGROUPS,NPTS,P,DATUM,
     +SIGMA,FLAT,BASE,MAX,SUMFIX)
      INTEGER P
      REAL F(68000),BASE(68000),OX(68000),CGRAD(68000),
     +SGRAD(68000),XI(68000,3),ETA(68000,3),S1(3),S2(3,3),
     +C1(3),C2(3,3),BETA(3),SIGMA(P),DATUM(P)
	 character*2048 str
      COMMON/SPACE/CHISQ,CHTARG,CHIZER,XSUM,BETA,C1,C2,S1,S2,BLANK
      common/savetime/ngo,i2pwr
      common/fac/factor,facdef,facfake,ratio
      common/heritage/iter
	common/MaxPage/n,f
      LOGICAL SUMFIX
c     
      BLANK=FLAT
      IF(BLANK.EQ.0.)GOTO 7
      DO 3 I=1,N
3     BASE(I)=BLANK
      GOTO 5
7     SUM=0.
      DO 4 I=1,N
4     SUM=SUM+BASE(I)
      BLANK=SUM/N
5     CHIZER=FLOAT(P)
      CHTARG=CHIZER
      ITER=0
      M=3
      if( ngo.gt.0 ) then
        iter=1
        goto 6
      endif
      DO 8 I=1,N
8     F(I)=BASE(I)
6     CALL OPUS(NGROUPS,NPTS,N,P,F,OX)
      CHISQ=0.
      DO 10 J=1,P
      A=OX(J)-DATUM(J)
      CHISQ=CHISQ+A*A/(SIGMA(J)*SIGMA(J))
10    OX(J)=2.*A/(SIGMA(J)*SIGMA(J))
      CALL TROPUS(NGROUPS,NPTS,N,P,OX,CGRAD)
      XSUM=0.
      TEST=0.
      SNORM=0.
      CNORM=0.
      TNORM=0.
      DO 12 I=1,N
      XSUM=XSUM+F(I)
      SGRAD(I)=-ALOG(F(I)/BASE(I))/BLANK
      SNORM=SNORM+SGRAD(I)*SGRAD(I)*F(I)
      CNORM=CNORM+CGRAD(I)*CGRAD(I)*F(I)
12    TNORM=TNORM+SGRAD(I)*CGRAD(I)*F(I)
      SNORM=SQRT(SNORM)
      CNORM=SQRT(CNORM)
      A=1.
      B=1./CNORM
      C=1./CNORM
      IF(ITER.NE.0)TEST=SQRT(0.5*abs(1.-TNORM/(SNORM*CNORM)))
      if(test.lt..0000001) test=.0000001
      IF(ITER.NE.0)A=1./(SNORM*2.*TEST)
      IF(ITER.NE.0)B=1./(CNORM*2.*TEST)
      DO 13 I=1,N
      XI(I,1)=F(I)*C*CGRAD(I)
13    XI(I,2)=F(I)*(A*SGRAD(I)-B*CGRAD(I))
      IF(SUMFIX)CALL PROJECT(1,N,XI)
      IF(SUMFIX)CALL PROJECT(2,N,XI)
      CALL OPUS(NGROUPS,NPTS,N,P,XI(1,1),ETA(1,1))
      CALL OPUS(NGROUPS,NPTS,N,P,XI(1,2),ETA(1,2))
      DO 14 J=1,P
14    OX(J)=ETA(J,2)/(SIGMA(J)*SIGMA(J))
      CALL TROPUS(NGROUPS,NPTS,N,P,OX,XI(1,3))
      A=0.
      DO 15 I=1,N
      B=F(I)*XI(I,3)
      A=A+B*XI(I,3)
15    XI(I,3)=B
      A=1./SQRT(A)
      DO 16 I=1,N
16    XI(I,3)=A*XI(I,3)
      IF(SUMFIX)CALL PROJECT(3,N,XI)
      CALL OPUS(NGROUPS,NPTS,N,P,XI(1,3),ETA(1,3))
      DO 17 K=1,M
      S1(K)=0.
      C1(K)=0.
      DO 18 I=1,N
      S1(K)=S1(K)+XI(I,K)*SGRAD(I)
18    C1(K)=C1(K)+XI(I,K)*CGRAD(I)
17    C1(K)=C1(K)/CHISQ
      DO 19 K=1,M
      DO 19 L=1,K
      S2(K,L)=0.
      C2(K,L)=0.
      DO 20 I=1,N
20    S2(K,L)=S2(K,L)-XI(I,K)*XI(I,L)/F(I)
      DO 21 J=1,P
21    C2(K,L)=C2(K,L)+ETA(J,K)*ETA(J,L)/(SIGMA(J)*SIGMA(J))
      S2(K,L)=S2(K,L)/BLANK
19    C2(K,L)=2.*C2(K,L)/CHISQ
      C2(1,2)=C2(2,1)
      C2(1,3)=C2(3,1)
      C2(2,3)=C2(3,2)
      S2(1,2)=S2(2,1)
      S2(1,3)=S2(3,1)
      S2(2,3)=S2(3,2)
      S=0.
      DO 22 I=1,N
22    S=S-F(I)*ALOG(F(I)/(BASE(I)*2.7182818285))/(BLANK*2.7182818285)
      A=S*BLANK*2.7182818285/XSUM
      write(str,103)ITER,TEST,S,CHTARG,CHISQ,XSUM
		call module_print(TRIM(str))
103   FORMAT(I3,4X,5(E10.4,2X))
      BETA(1)=-0.5*C1(1)/C2(1,1)
      BETA(2)=0.
      BETA(3)=0.
      IF(ITER.NE.0)CALL MOVE(3,sigma,p)
      A=0.
      DO 23 I=1,N
      DO 24 K=1,M
24    F(I)=F(I)+BETA(K)*XI(I,K)
      IF(F(I).LT.0.)F(I)=1.E-3*BLANK
      A=A+F(I)
23    CONTINUE
      IF(.NOT.SUMFIX)GOTO 50
      DO 51 I=1,N
51    F(I)=F(I)/A
50    ITER=ITER+1
      if(iter.le.1) goto 6
      IF(TEST.LT.0.02.AND.ABS(CHISQ/CHIZER-1.).LT.0.01.OR.ITER.GT.
     *MAX) THEN
c        write(99,98)
c        write(99,99)(F(I),I=1,N,4)
        RETURN
      ENDIF
99    FORMAT(1X,5E13.4)
98    FORMAT(1X,//,1X,'FREQUENCY SPECTRUM')
      GOTO 6
      END
"""
# common /SPACE/ is used only here and below
# max changed to itermax to avoid overwriting built in function
def MAXENT(datum,sigma,flat,base,itermax,sumfix,SAVETIME_ngo,MAXPAGE_n,MAXPAGE_f,PULSESHAPE_convol,
		DETECT_a,DETECT_b,DETECT_e,FAC_factor,FAC_facfake,SAVETIME_i2,mylog,prog):
	npts,ngroups=datum.shape
	p=npts*ngroups
	xi=np.zeros([MAXPAGE_n,3])
	eta=np.zeros([npts,ngroups,3])
	#
	SPACE_blank=flat
	if(SPACE_blank!=0):
		base=np.zeros([MAXPAGE_n])
		base[:]=SPACE_blank
	else:
		SPACE_blank=np.mean(base)
	SPACE_chizer=float(p)
	SPACE_chtarg=SPACE_chizer
	HERITAGE_iter=0
	m=3
	if(SAVETIME_ngo>0):
		HERITAGE_iter=1
	else:
		MAXPAGE_f=np.array(base)
	test=99. # temporary for 1st test
	SPACE_chisq=SPACE_chizer*2. # temporary for 1st test
	mylog.debug("entering loop with spectrum from {0} to {1}".format(np.amin(MAXPAGE_f),np.amax(MAXPAGE_f)))
	while( HERITAGE_iter<=itermax and (HERITAGE_iter<=1 or not (test<0.02 and abs(SPACE_chisq/SPACE_chizer-1)<0.01) ) ): # label 6
		mylog.debug("start loop, iter={} ngo={} test={} chisq={}".format(HERITAGE_iter,SAVETIME_ngo,test,SPACE_chisq/SPACE_chizer))
		mylog.debug("entering loop with spectrum from {0} to {1}".format(np.amin(MAXPAGE_f),np.amax(MAXPAGE_f)))
		ox=OPUS(MAXPAGE_f,SAVETIME_i2,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e)
		if(not np.all(np.isfinite(ox))):
			mylog.warning("ox has some NaNs or Infs")
		mylog.debug("ox from {0} to {1}".format(np.amin(ox),np.amax(ox)))
		a=ox-datum
		SPACE_chisq=np.sum(a**2/sigma**2)
		ox=2*a/(sigma**2)
		cgrad=TROPUS(ox,SAVETIME_i2,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e)
		if(not np.all(np.isfinite(cgrad))):
			mylog.warning("cgrad has some NaNs or Infs")
		mylog.debug("cgrad from {0} to {1}".format(np.amin(cgrad),np.amax(cgrad)))
		SPACE_xsum=np.sum(MAXPAGE_f)
		sgrad=-np.log(MAXPAGE_f/base)/SPACE_blank
		if(not np.all(np.isfinite(sgrad))):
			mylog.warning("sgrad has some NaNs or Infs")
		mylog.debug("sgrad from {0} to {1}".format(np.amin(sgrad),np.amax(sgrad)))
		snorm=math.sqrt(np.sum(sgrad**2*MAXPAGE_f))
		cnorm=math.sqrt(np.sum(cgrad**2*MAXPAGE_f))
		tnorm=np.sum(sgrad*cgrad*MAXPAGE_f)
		mylog.debug("snorm={} cnorm={} tnorm={}".format(snorm,cnorm,tnorm))
		a=1.
		b=1./cnorm
		c=1./cnorm
		if(HERITAGE_iter!=0):
			test=math.sqrt(0.5*abs(1.-tnorm/(snorm*cnorm)))
			# also eliminate NaNs!
			if(test<1.E-7 or not np.isfinite(test)):
				test=1.E-7
			a=1./(snorm*2.*test)
			b=1./(cnorm*2.*test)
		else:
			test=0.
		mylog.debug("a={} b={} c={}".format(a,b,c))
		xi[:,0]=MAXPAGE_f*c*cgrad
		xi[:,1]=MAXPAGE_f*(a*sgrad-b*cgrad)
		if(not np.all(np.isfinite(xi[:,0]))):
			mylog.warning("xi[,0] has some NaNs or Infs")
		if(not np.all(np.isfinite(xi[:,1]))):
			mylog.warning("xi[,1] has some NaNs or Infs")
		if(sumfix):
			PROJECT(0,MAXPAGE_n,xi)
			PROJECT(1,MAXPAGE_n,xi)
		eta[:,:,0]=OPUS(xi[:,0],SAVETIME_i2,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e)
		eta[:,:,1]=OPUS(xi[:,1],SAVETIME_i2,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e)
		if(not np.all(np.isfinite(eta[:,:,0]))):
			mylog.warning("eta[,,0] has some NaNs or Infs")
		if(not np.all(np.isfinite(eta[:,:,1]))):
			mylog.warning("eta[,,1] has some NaNs or Infs")
		ox=eta[:,:,1]/(sigma**2)
		xi[:,2]=TROPUS(ox,SAVETIME_i2,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e)
		if(not np.all(np.isfinite(xi[:,2]))):
			mylog.warning("xi[,2] has some NaNs or Infs")
		a=1./math.sqrt(np.sum(xi[:,2]**2*MAXPAGE_f))
		xi[:,2]=xi[:,2]*MAXPAGE_f*a
		if(sumfix):
			PROJECT(2,MAXPAGE_n,xi)
		eta[:,:,2]=OPUS(xi[:,2],SAVETIME_i2,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e)
		if(not np.all(np.isfinite(eta[:,:,2]))):
			mylog.warning("eta[,,2] has some NaNs or Infs")
		# loop DO 17, DO 18
		SPACE_s1=np.dot(sgrad,xi)
		SPACE_c1=np.dot(cgrad,xi)/SPACE_chisq
		# loops DO 19,DO 20, DO 21 harder to completely remove
		SPACE_s2=np.zeros([3,3])
		SPACE_c2=np.zeros([3,3])
		for L in range(m):
			for k in range(m):
				if(L<=k):
					SPACE_s2[k,L]=-np.sum(xi[:,k]*xi[:,L]/MAXPAGE_f)/SPACE_blank
					SPACE_c2[k,L]=np.sum(eta[:,:,k]*eta[:,:,L]/sigma**2)*2./SPACE_chisq
				else: # make symmetric
					SPACE_s2[k,L]=SPACE_s2[L,k]
					SPACE_c2[k,L]=SPACE_c2[L,k]
		s=-np.sum(MAXPAGE_f*np.log(MAXPAGE_f/(base*math.e)))/(SPACE_blank*math.e) # spotted missing minus sign!
		a=s*SPACE_blank*math.e/SPACE_xsum
		mylog.notice("{:3}    {:10.4}  {:10.4}  {:10.4}  {:10.4}  {:10.4}".format(HERITAGE_iter,test,s,SPACE_chtarg,SPACE_chisq,SPACE_xsum))
		SPACE_beta=np.array([-0.5*SPACE_c1[0]/SPACE_c2[0,0],0.0,0.0])
		if(not np.all(np.isfinite(SPACE_beta))):
			mylog.warning("SPACE_beta has some NaNs or Infs")
		if(HERITAGE_iter!=0):
			(sigma,SPACE_chtarg,SPACE_beta,FAC_factor,FAC_facfake)=MOVE(
				sigma,SPACE_chisq,SPACE_chizer,SPACE_xsum,SPACE_c1,SPACE_c2,SPACE_s1,SPACE_s2,SPACE_blank,FAC_factor,FAC_facfake,mylog)
		# do 23,24
		MAXPAGE_f += np.dot(xi,SPACE_beta)
		MAXPAGE_f=np.where(MAXPAGE_f<0,1.E-3*SPACE_blank,MAXPAGE_f)
		a=np.sum(MAXPAGE_f)
		if(sumfix):
			MAXPAGE_f /= a
		# 50
		HERITAGE_iter +=1
		prog.report("chisq="+str(SPACE_chisq))
		#if(iter>1):
		#	if(test<0.02 and abs(chisq/chizer-1)<0.01) or HERITAGE_iter>max):
		#		break
			
	return (sigma,base,HERITAGE_iter,MAXPAGE_f,FAC_factor,FAC_facfake)
	
