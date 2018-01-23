import numpy as np

# translation of opus.for
"""
      SUBROUTINE OPUS(NGROUPS,NPTS,N,P,X,OX)
      INTEGER P
      REAL X(4096),OX(NPTS,NGROUPS)
      REAL Y(68000)
      common/savetime/ngo,i2pwr
      COMMON/MISSCHANNELS/MM
      common/pulseshape/convolR(8192),CONVOLI(8192)
      COMMON/DETECT/A,B,E,c,d
      REAL A(64),B(64),c(64),d(64),E(8192)
      DO 1 I=1,68000
1     Y(I)=0.
      DO 2 I=1,N
      Y(I+I)=X(I)*CONVOLI(I)
2     Y(I+I-1)=X(I)*convolR(I)
      CALL FFT(i2pwr+1,Y,1.)
      DO 3 J=1,NGROUPS
      DO 3 K=1,npts
3     OX(K,J)=(A(J)*Y(K+K-1)+B(J)*Y(K+K))*E(K)
      RETURN
      END
"""
def OPUS(x,SAVETIME_i2,PULSESHAPE_convol,DETECT_a,DETECT_b,DETECT_e):
	ngroups=DETECT_a.shape[0]
	npts=DETECT_e.shape[0]
	n=x.shape[0]
	y=np.zeros([SAVETIME_i2],dtype=np.complex_)
	y[:n]=x*PULSESHAPE_convol
	y2=np.fft.ifft(y)*SAVETIME_i2 # SN=+1, inverse FFT without the 1/N
	ox=(np.outer(np.real(y2[:npts]),DETECT_a)+np.outer(np.imag(y2[:npts]),DETECT_b))*DETECT_e[:,np.newaxis]
	return ox
	
	