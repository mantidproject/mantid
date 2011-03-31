from mantidsimple import *

def heliumDetectorEff(workspace):
  ''' 	Calculate the corrected Helium detector values. '''
  
  ac= 6.022169e23		# Avogadro's constant mol-1
  vm= 2.24136e4			# Molar volume of gas cm3 mol-1
  gp= 10.0			# Gas pressure (atms)
  gsig0= 5333.0e-24		# Gas cross section at LAM0 cm2
  gt= 2.5			# Gas path length cm
  lam0= 1.8			# Characteristic wavelength
  
  gn= ac*gp/vm			# Number density of gas
  sgn= gn*gsig0*gt/lam0	        # Exponential term for gas

  OneMinusExponentialCor(workspace,workspace,str(sgn),Operation="Divide")
  
  wt= 60.014			# Molecular weight Ni-Cu g mol-1
  rho= 8.00			# Density Ni-Cu g cm-3
  ct= 0.05			# Monel (Ni-Cu) wall thickness cm
  wsig0= 4.522e-24		# Wall cross section at LAM0 cm2
  
  wn= ac*rho/wt			# Number density of wall
  swn= wn*wsig0*ct/lam0	        # Exponential term for wall
  
  ExponentialCorrection(workspace,workspace,C1=str(swn),Operation="Divide")
  
  # simple polynomial correction based on a D2O spectrum taken at 1.5 deg
  PolynomialCorrection(workspace,workspace,"-1.3697,0.8602,-0.7839,0.2866,-0.0447,0.0025")
  return

def monitor2Eff(workspace):
  ''' Calculate the corrected monitor2 values. '''
  # expon= unt*(1-exp(-8.3047 * zz * x_mean ))
  #	yout[i]= yin[i]*(1.0-expon) / expon
  #	eout[i]= ein[i]*(1.0-expon) / expon
  # The above correction is equivalent to: (1/unt - 1) + e^(-8.3047*zz*x)
  #                                        ------------------------------
  #                                           ( 1 - e^(-8.3047*zz*x) )
  unt=0.24   # 0.05		# ratio of scintillator to total area
  zz = 0.6    #0.03		# thickness(cm) of scintillator
  c1 = 0.7112*zz      #8.3047*zz
  
  ExponentialCorrection(workspace,workspace,C1=str(c1),Operation="Multiply")
  shift = (1.0/unt)-1.0
  CreateSingleValuedWorkspace("shift",str(shift),"0.0")
  Plus(workspace,"shift",workspace)
  mtd.deleteWorkspace("shift")
  OneMinusExponentialCor(workspace,workspace,str(c1))

  return


def main():
  '''This main routine. It is executed on if the script is run directly, not if it is imported.''' 
  LoadRawDialog(OutputWorkspace="ws",SpectrumMin="1",SpectrumMax="1")
  ConvertUnits("ws","ws","Wavelength",AlignBins="1")
  heliumDetectorEff("ws")
  monitor2Eff("ws")
  print "Done!"


if __name__ == '__main__':
  main()

