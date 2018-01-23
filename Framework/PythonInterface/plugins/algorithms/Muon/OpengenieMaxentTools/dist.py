import numpy as np

# translation of dist.for
"""
      FUNCTION DIST(M)
      REAL BETA(3),S1(3),S2(3,3),C1(3),C2(3,3)
      COMMON/SPACE/CHISQ,CHTARG,CHIZER,XSUM,BETA,C1,C2,S1,S2,BLANK
      W=0.
      DO 26 K=1,M
      Z=0.
      DO 27 L=1,M
27    Z=Z-S2(K,L)*BETA(L)
26    W=W+BETA(K)*Z
      DIST=W
      RETURN
      END
"""
# inner loop Z=sum_L(S2(K,L)*beta_L)
# outer loop W=sum(beta(k)*Z(k))
def DIST(SPACE_beta,SPACE_s2):
	return -np.dot(SPACE_beta,np.dot(SPACE_s2,SPACE_beta))
	