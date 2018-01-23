import numpy as np

# translated from project.for
"""
      SUBROUTINE PROJECT(K,N,XI)
      REAL XI(N,3)
      A=0.
      DO 53 I=1,N
      A=A+XI(I,K)
53    CONTINUE
      A=A/N
      DO 54 I=1,N
      XI(I,K)=XI(I,K)-A
54    CONTINUE
      RETURN
      END
"""
def PROJECT(k,n,xi): # xi modified in place
	a=np.sum(xi[:n,k])
	a=a/n
	xi[:n,k]-=a
