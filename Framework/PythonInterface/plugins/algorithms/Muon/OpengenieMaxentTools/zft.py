import numpy as np
# translation of zft.for
"""
      SUBROUTINE ZFT(P,N,F,ZR,ZI)
      REAL F(68000),Y(68000)
      REAL ZR(8192),ZI(8192)
      common/points/npts,ngroups,nhists
      common/pulseshape/convolR(8192),CONVOLI(8192)
      common/savetime/ngo,i2pwr
      COMMON/DETECT/A,B,E,c,d
      REAL A(64),B(64),c(64),d(64),E(8192)
      DO 1 I=1,68000
1     Y(I)=0.
      DO 2 I=1,N
      Y(I+I)=F(I)*CONVOLI(I)
2     Y(I+I-1)=F(I)*convolR(i)
      CALL FFT(i2pwr+1,Y,1.)
      DO 3 I=1,npts
      ZR(I)=E(I)*Y(I+I-1)
3     ZI(I)=E(I)*Y(I+I)
      RETURN
      END
"""
# param P not used!
# N is number of frequency points provided
# 2**I2PWR is length of FFT (zero pad spectrum). Replace with I2
# npts is number of time bins (truncate result)
# PULSESHAPE_convol is complex (convolR + i*convolI)
def ZFT(f,PULSESHAPE_convol,DETECT_e,SAVETIME_i2):
	n=f.shape[0]
	npts=DETECT_e.shape[0]
	y=np.zeros([SAVETIME_i2],dtype=np.complex_)
	y[:n]=f*PULSESHAPE_convol
	y2=np.fft.ifft(y)*SAVETIME_i2 # SN=+1 meaning inverse FFT without the 1/N scale factor
	return np.real(y2[:npts])*DETECT_e,np.imag(y2[:npts])*DETECT_e
