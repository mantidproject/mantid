import numpy as np


def PROJECT(k,n,xi): # xi modified in place
	a=np.sum(xi[:n,k])
	a=a/n
	xi[:n,k]-=a
