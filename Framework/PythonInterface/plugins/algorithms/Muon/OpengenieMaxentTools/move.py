import math
from chinow import CHINOW
from dist import DIST

# translation of move.for
"""
      SUBROUTINE MOVE(M,sigma,p)
      REAL BETA(3),S1(3),C1(3),S2(3,3),C2(3,3),sigma(1)
	character*255 str
      COMMON/SPACE/CHISQ,CHTARG,CHIZER,XSUM,BETA,C1,C2,S1,S2,BLANK
      common/fac/factor,facdef,facfake,ratio
      integer p
      CMIN=CHINOW(0.,M)
      IF(CMIN*CHISQ.GT.CHIZER)CTARG=0.5*(1.+CMIN)
      IF(CMIN*CHISQ.LE.CHIZER)CTARG=CHIZER/CHISQ
      A1=0.
      A2=1.
      jtest=0
      F1=CMIN-CTARG
      F2=CHINOW(1.,M)-CTARG
41    ANEW=0.5*(A1+A2)
      FX=CHINOW(ANEW,M)-CTARG
      IF(F1*FX.GT.0.)A1=ANEW
      IF(F1*FX.GT.0.)F1=FX
      IF(F2*FX.GT.0.)A2=ANEW
      IF(F2*FX.GT.0.)F2=FX
      IF(ABS(FX).GE.1.E-3) then
        jtest=jtest+1
        if(jtest.gt.10000) then
          write(str,*) ' stuck in MOVE : chi**2 not tight enough'
		call module_print(TRIM(str))
          do 222 i=1,p
222       sigma(i)=sigma(i)*.99
          factor=factor*.99
          facdef=factor
          facfake=facfake*.99
          write(str,333) factor
333       format(' tightening looseness factor by 1 % to:',f5.3)
	    call module_print(TRIM(str))
          goto 444
        endif
        GOTO 41
      endif
444      W=DIST(M)
      IF(W.LE.0.1*XSUM/BLANK)GOTO 42
      DO 44 K=1,M
44    BETA(K)=BETA(K)*SQRT(0.1*XSUM/(BLANK*W))
42    CHTARG=CTARG*CHISQ
      RETURN
      END
"""
# variable p is used? Size of sigma

def MOVE(sigma,SPACE_chisq,SPACE_chizer,SPACE_xsum,SPACE_c1,SPACE_c2,SPACE_s1,SPACE_s2,SPACE_blank,FAC_factor,FAC_facfake,mylog):
	(cmin,SPACE_beta)=CHINOW(0.,SPACE_c1,SPACE_c2,SPACE_s1,SPACE_s2,mylog)
	if(cmin*SPACE_chisq > SPACE_chizer):
		ctarg=0.5*(1.+cmin)
	else:
		ctarg=SPACE_chizer/SPACE_chisq
	a1=0.
	a2=1.
	jtest=0.
	f1=cmin-ctarg
	(f2,SPACE_beta)=CHINOW(1.,SPACE_c1,SPACE_c2,SPACE_s1,SPACE_s2,mylog)
	f2=f2-ctarg
	while(True):
		anew=0.5*(a1+a2)
		(fx,SPACE_beta)=CHINOW(anew,SPACE_c1,SPACE_c2,SPACE_s1,SPACE_s2,mylog)
		fx=fx-ctarg
		if(f1*fx>0):
			a1=anew
			f1=fx
		if(f2*fx>0):
			a2=anew
			f2=fx
		if(abs(fx)>=1.E-3):
			jtest=jtest+1
			if(jtest>10000):
				mylog.notice(' stuck in MOVE : chi**2 not tight enough')
				sigma=sigma*0.99
				FAC_factor=FAC_factor*0.99
				#FAC_facdef=FAC_factor
				FAC_facfake=FAC_facfake*0.99
				mylog.notice(' tightening looseness factor by 1 % to: {0}'.format(FAC_factor))
				break
		else:
			break
	w=DIST(SPACE_beta,SPACE_s2)
	if(w>0.1*SPACE_xsum/SPACE_blank):
		SPACE_beta=SPACE_beta*math.sqrt(0.1*SPACE_xsum/(SPACE_blank*w))
	SPACE_chtarg=ctarg*SPACE_chisq
	return sigma,SPACE_chtarg,SPACE_beta,FAC_factor,FAC_facfake
	