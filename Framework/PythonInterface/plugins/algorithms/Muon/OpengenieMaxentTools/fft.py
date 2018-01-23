# translation of FFT.for
import math
import numpy as np


"""
      SUBROUTINE FFT(KK,A,SN)
      COMPLEX A(1),B,EQ,EL,EN
      DIMENSION INU(16)
      DATA INU/1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,
     + 16384,32768/
      N=INU(KK+1)
      N2=N/2
      E=SN*3.1415926536
      J=1+N2
      DO 1 I=2,N
      K=KK-1
      M=N2
      IF(I-J)2,4,4
2     B=A(J)
      A(J)=A(I)
      A(I)=B
4     L=J-M
      IF(L)5,5,3
3     J=L
      M=INU(K)
      K=K-1
      IF(K)5,5,4
5     J=J+M
1     CONTINUE
      K=1
6     IF(K-N)9,8,8
9     L=K+K
      C2=COS(E)
      EQ=CMPLX(C2,SIN(E))
      EL=(1.,0.)
      C2=C2+C2
      DO 10 M=1,K
      DO 7 I=M,N,L
      J=I+K
      B=EL*A(J)
      A(J)=A(I)-B
      A(I)=A(I)+B
7     CONTINUE
      EN=EQ*C2-EL
      X=REAL(EN)
      Y=AIMAG(EN)
      EN=EN*.5*(3.-X*X-Y*Y)
      EL=EQ
      EQ=EN
10    CONTINUE
      K=L
      E=0.5*E
      GO TO 6
8     CONTINUE
      RETURN
      END
"""
def fortran_FFT(kk,a,sn):
	# INU=2**(n-1) in Fortran terms
	n=2*kk # n=orig, length of data
	n2=n/2 # n2 orig
	e=sn*math.pi
	j=n2 # now reduced by 1
	# loop DO 1
	for i in range(1,n): # i less by 1
		k=kk-1 # orig, log2(something)
		m=n2 # orig
		if(i-j)<0: # both less 1
			print "swapping",i," with ",j
			b=a[j] # label 2
			a[j]=a[i]
			a[i]=b
		while(True): # label 4
			L=j-m # L reduced by 1
			if(L>=0):
				# label 3
				j=L # reduced by 1
				m=2**(k-1) # orig
				k=k-1 # orig, log2
				if(k<=0):
					# goto label 5
					break
				# else continue loop label 4
			else:
				break
		j=j+m # label 5. orig m, reduced j
	# 1 continue
	k=1
	while(k<n):
		L=k+k
		c2=math.cos(e)
		eq=complex(c2,math.sin(e))
		el=1.
		c2=c2+c2
		for m in range(k): # DO 10..
			for i in range(m,n,L): # DO 7
				j=i+k
				b=el*a[j]
				a[j]=a[i]-b
				a[i]=a[i]+b
			en=eq*c2-el
			x=en.real
			y=en.imag
			en=en*0.5*(3.-x*x-y*y)
			el=eq
			eq=en
		k=L
		e=0.5*e
	return a

# test
#arr=np.random.uniform(-1.0,1.0,size=16)+1.j*np.random.uniform(-1.0,1.0,size=16)
arr=np.zeros([16],dtype=np.complex)
arr[1]=complex(1.0,2.0)
print arr



numpyfft=np.fft.ifft(arr)
fortranfft=fortran_FFT(4,arr,1.0)
for i in range(16):
	print numpyfft[i]*16
print fortranfft

# above Python code not yet working
# comparison between numpy and original FORTRAN running under Intel FORTRAN compiler:

# FORTRAN FFT(kk,x,-1) == np.fft.fft(x)

# FORTRAN FFT(kk,x,+1) / N == np.fft.ifft(x)
