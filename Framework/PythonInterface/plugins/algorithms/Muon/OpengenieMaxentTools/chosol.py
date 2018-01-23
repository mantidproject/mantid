import numpy as np
import math
# translation of chosol.for
"""
      SUBROUTINE CHOSOL(A,B,N,X)
      REAL L(3,3),A(3,3),BL(3),B(3),X(3)
      L(1,1)=SQRT(A(1,1))
      DO 35 I=2,N
      L(I,1)=A(I,1)/L(1,1)
      DO 35 J=2,I
      Z=0.
      DO 36 K=1,J-1
36    Z=Z+L(I,K)*L(J,K)
      Z=A(I,J)-Z
C       TRAP FOR NEGATIVE SQUARE ROOT
      IF(Z.LE.0.)Z=1.E-10
      IF(J.EQ.I)L(I,J)=SQRT(Z)
      IF(J.NE.I)L(I,J)=Z/L(J,J)
35    CONTINUE
      BL(1)=B(1)/L(1,1)
      DO 37 I=2,N
      Z=0.
      DO 38 K=1,I-1
38    Z=Z+L(I,K)*BL(K)
37    BL(I)=(B(I)-Z)/L(I,I)
      X(N)=BL(N)/L(N,N)
      DO 39 I1=1,N-1
      I=N-I1
      Z=0.
      DO 40 K=I+1,N
40    Z=Z+L(K,I)*X(K)
39    X(I)=(BL(I)-Z)/L(I,I)
      RETURN
      END
"""
def CHOSOL_old(a,b,n):
	# a,b: arrays
	# n: size (to use)
	# returns x (array) (was parameter by reference, modified in place)
	assert(n==3)
	assert(a.shape==(3,3))
	assert(b.shape==(3,))
	L=np.zeros([n,n])
	L[0,0]=math.sqrt(a[0,0])
	for i in range(1,n):
		L[i,0]=a[i,0]/L[0,0]
		for j in range(1,i+1):
			z=a[i,j]-np.dot(L[i,:j],L[j,:j])
			if(z<0):
				raise UserWarning("trapped a negative square root in CHOSOL_old")
				z=1.E-10
			if(j==i):
				L[i,j]=math.sqrt(z)
			else:
				L[i,j]=z/L[j,j]

	#print L

	bl=np.zeros([n])
	bl[0]=b[0]/L[0,0]
	for i in range(1,n):
		z=np.dot(L[i,:i],bl[:i])
		bl[i]=(b[i]-z)/L[i,i]
	x=np.zeros([n])
	x[n-1]=bl[n-1]/L[n-1,n-1]
	for i in range(n-2,-1,-1):
		z=np.dot(L[i+1:n,i],x[i+1:n])
		x[i]=(bl[i]-z)/L[i,i]
	return x

# solves ax=b by Cholesky decomposition
# only uses lower half of array a
# equivalent to np.linalg.solve(a,b) IF a is actually symmetric (Hermitian)
# exact equivalent code follows:
def CHOSOL(a,b,mylog):
	n=a.shape[0]
	try:
		L=np.linalg.cholesky(a)
	except:
		mylog.warning("np.linalg.cholesky failed, trying backup")
		mylog.debug("array a is "+str(a))
		L=np.zeros([n,n])
		L[0,0]=math.sqrt(a[0,0])
		for i in range(1,n):
			L[i,0]=a[i,0]/L[0,0]
			for j in range(1,i+1):
				z=a[i,j]-np.dot(L[i,:j],L[j,:j])
				if(z<0):
					mylog.warning("trapped a negative square root in CHOSOL backup code")
					z=1.E-10
				if(j==i):
					L[i,j]=math.sqrt(z)
				else:
					L[i,j]=z/L[j,j]
		
	bl=np.zeros([n])
	for i in range(n):
		bl[i]=(b[i]-np.dot(L[i,:],bl))/L[i,i]
	x=np.zeros([n])
	for i in range(n-1,-1,-1):
		x[i]=(bl[i]-np.dot(L[:,i],x))/L[i,i]
	return x

# tests
if(__name__ == "__main__"):
	a=np.array([[11.0,3.7,2.9],[3.7,19.3,2.2],[2.9,2.2,7.1]]) # Hermitian!
	b=np.array([2.5,1.2,4.8])

	x1=CHOSOL0(a,b,3)
	print np.linalg.cholesky(a)
	x2=np.linalg.solve(a,b)
	x3=CHOSOL(a,b)
	print x1
	print x2
	print x3
	print

	a1=np.array(a)
	a2=np.array(a)
	a1[0,2]=1.9
	print "alter 0,2"
	print a1
	x11=CHOSOL0(a1,b,3)
	print np.linalg.cholesky(a1)
	print x11
	print np.linalg.solve(a1,b)
	a2[2,0]=1.9
	print "alter 2,0"
	x22=CHOSOL0(a2,b,3)
	print np.linalg.cholesky(a2)
	print x22
	print np.linalg.solve(a2,b)
