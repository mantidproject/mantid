from numpy import *

x256=arange(256)
x=x256/256.
ri=zeros(256)
bi=array(256)
gi=array(256)

ri=-4*x+4.5
ri[x<0.75]=4*x[x<0.75]-1.5
ri[ri<0]=0
ri=ri*256
ri[ri>255]=255



gi=-4*x+3.5
gi[x<0.5]=4*x[x<0.5]-0.5
gi[gi<0]=0
gi=gi*256
gi[gi>255]=255



bi=-4*x+2.5
bi[x<0.25]=4*x[x<0.25]+0.5
bi[bi<0]=0
bi=bi*256
bi[bi>255]=255

