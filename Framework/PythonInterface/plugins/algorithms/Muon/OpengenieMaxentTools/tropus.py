import numpy as np

# translation of TROPUS.FOR
"""
      SUBROUTINE TROPUS(NGROUPS,NPTS,N,P,OX,X)
      INTEGER P
      REAL X(4096),OX(NPTS,NGROUPS)
      REAL Y(68000)
      common/savetime/ngo,i2pwr
      common/pulseshape/convolR(8192),CONVOLI(8192)
      COMMON/MISSCHANNELS/MM
      COMMON/DETECT/A,B,E,c,d
      REAL A(64),B(64),c(64),d(64),E(8192)
8     DO 1 I=1,68000
1     Y(I)=0.0
      DO 2 K=1,npts
      SR=0.
      SI=0.
      DO 3 J=1,NGROUPS
      SR=SR+A(J)*OX(K,J)
3     SI=SI+B(J)*OX(K,J)
      Y(K+K-1)=SR*E(K)
      Y(K+K)=SI*E(K)
2     CONTINUE
      CALL FFT(i2pwr+1,Y,-1.)
      DO 4 I=1,N
4     X(I)=(Y(I+I-1)*convolR(I)+Y(I+I)*CONVOLI(I))
      RETURN
      END
"""
def TROPUS(ox,SAVETIME_i2,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e):
	npts,ngroups=ox.shape
	n=PULSESHAPE_convol.shape[0]
	y=np.zeros([SAVETIME_i2],dtype=np.complex_)
	y[:npts]=np.dot(ox,DETECT_a)*DETECT_e+1.j*np.dot(ox,DETECT_b)*DETECT_e
	y2=np.fft.fft(y) # SN=-1 meaning forward fft, scale is OK
	x=np.real(y2)[:n]*np.real(PULSESHAPE_convol)+np.imag(y2)[:n]*np.imag(PULSESHAPE_convol)
	
	return x
	