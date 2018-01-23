import numpy as np
# translation of back.for
"""
      SUBROUTINE BACK(HISTS,NGROUPS,NPTS,P,DATUM,SIGMA,CORR)
      INTEGER P
      REAL ZR(8192),ZI(8192),AMP(64),PH(64)
      REAL CORR(NPTS,NGROUPS),HISTS(NGROUPS)
      COMMON/MISSCHANNELS/MM
      COMMON/FLAGS/FITDEAD,FIXPHASE,FITAMP
      COMMON/RUNDATA/RES,IRUNNO,IFRAM
      COMMON/DETECT/A,B,E,c,d
      COMMON/FASE/PHASE(64),phshift
      common/sense/phi(64),TAUD(64)
      REAL A(64),B(64),c(64),d(64),E(8192),taud
      REAL DATUM(NPTS,NGROUPS),SIGMA(NPTS,NGROUPS)
      CHARACTER*1 FITDEAD,FIXPHASE,FITAMP
	CHARACTER*2046 str
      DATA AMP /1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,
     &          1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,48*1.0/
      DO 2 J=1,NGROUPS

      IF(HISTS(J).EQ.0) THEN
       GOTO 5
      ENDIF
      Ax=0.
      Bx=0.
      DO 3 I=1,npts
      Ax=Ax+E(I)*DATUM(I,J)/(SIGMA(I,J)*SIGMA(I,J))
3     Bx=Bx+E(I)*E(I)/(SIGMA(I,J)*SIGMA(I,J))
      SCALE=Ax/Bx
5     d(j)=scale
      DO 4 I=1,npts
      IF(sigma(i,j).gt.1e6) goto 4
      DATUM(I,J)=DATUM(I,J)-SCALE*E(I)
4     continue
2     CONTINUE
c      write(99,*) ' DATUM AND EXP...'
c      write(99,*) (DATUM(100,l),l=1,4)
c      write(99,*) (E(100)*D(L),l=1,4)
C       FIRST APPROXIMATION TO AMPLITUDE : PROP TO EXP   
      GGRO=0.
      SUM=0.
      DO 10 J=1,NGROUPS
      IF(HISTS(J).EQ.0)GOTO 10
      GGRO=GGRO+1.
      SUM=SUM+D(J)
10    CONTINUE
      SUM=SUM/GGRO
      DO 11 J=1,NGROUPS
      IF(HISTS(J).EQ.0)THEN
        AMP(J)=0.0
      ELSE
        AMP(J)=D(J)/SUM
      ENDIF
11    CONTINUE
      OPEN(15,FILE='PHASE.DAT',STATUS='OLD')  
      DO 1 I=1,NGROUPS
      READ(15,*) PH(I)
      IF(HISTS(I).EQ.0)GOTO 1
      PHASE(I)=PH(I)/57.296
      A(I)=AMP(I)*COS(PHASE(I))
      B(I)=AMP(I)*SIN(PHASE(I))
1     CONTINUE
c      write(99,*) ' PHASES AS LOADED...'

	call module_print("")
	call module_print(" PHASES AS LOADED...")
	call module_print("")
	write(str,*) (PH(I),I=1,NGROUPS)


123   FORMAT(A10)
	call module_print(TRIM(str))
      CLOSE(15)
c     write(99,*) ' EXPONENTIALS AS FITTED...'
c     write(99,*) (D(J),J=1,NGROUPS)
      call module_print(" EXPONENTIALS AS FITTED...")
	write(str,*) (D(J),J=1,NGROUPS)
	call module_print(TRIM(str))
      RETURN
      END
"""
def BACK(hists,datum,sigma,DETECT_e,filePHASE,mylog):
	# ngroups, npts are array sizes
	# hists(ngroups): integer flag to say if used or not? R
	# P (integer) - not used?
	# Datum (npts,ngroups) = counts, R (and occasionally W)
	# Sigma (npts,ngroups) = error values R
	# corr(npts,ngroups) - not used?
	(npts,ngroups)=datum.shape
	DETECT_d=np.zeros([ngroups])
	for j in range(ngroups):
		if(hists[j]>0):
			Ax=np.sum(DETECT_e*datum[:,j]/sigma[:,j]**2)
			Bx=np.sum(DETECT_e**2/sigma[:,j]**2)
			scale=Ax/Bx
		DETECT_d[j]=scale
		datum[:,j]=np.where(sigma[:,j]>1.e6,datum[:,j],datum[:,j]-scale*DETECT_e)
	ggro=0.0
	sum=0.0
	for j in range(ngroups):
		if(hists[j]>0):
			ggro=ggro+1.0
			sum=sum+DETECT_d[j]
	sum=sum/ggro
	amp=np.where(hists>0,DETECT_d/sum,0.0)
	# read PHASE.DAT, was in degrees, one number per line. Present filePHASE is in radians
	FASE_phase=filePHASE
	DETECT_a=amp*np.cos(FASE_phase)
	DETECT_b=amp*np.sin(FASE_phase)
	# convert to self.log().information() if self is passed in
	mylog.debug("phases as loaded"+str(filePHASE))
	mylog.debug("exponentials as fitted"+str(DETECT_d))
	return datum,DETECT_a,DETECT_b,DETECT_d,FASE_phase