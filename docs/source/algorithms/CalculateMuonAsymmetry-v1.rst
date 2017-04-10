.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calculates the asymmetry from the specified muon
spectra. By default, all of the spectra
in a workspace will be corrected.

The formula for calculating the asymmetry (from counts) is given by:

.. math:: \textrm{NewData} = (\textrm{OldData}\times e^\frac{t}{\tau})/(F N_0) - 1.0,

where :math:`\tau` is the muon lifetime (2.1969811e-6 seconds), :math:'F' is the number of good frames and :math:`N_0` is a
fitted normalisation constant. The normalisation is calculated by fitting to the normalised counts which is given by

.. math:: \textrm{normalisedCounts}=(\textrm{OldData}\times e^\frac{t}{\tau})/F

and the fitting function is given by

.. math:: N_0[1+f(t)] 

where :math:`f(t)` is a user defined function. 

It is also possible to calculate the asymmetry from an estimated asymmetry. 

Usage
-----

**Example - Calculating Asymmetry From Counts:**
This example is for calculating the Asymmetry from counts.

.. testcode:: ExCounts

   import math
   import numpy as np
   xData=np.linspace(start=0,stop=10,num=22)   
   yData=[]
   tau =  2.1969811
   for x in xData:
        yData.append(50.*(1+10.*math.cos(3.*x))*math.exp(-x/tau))
   input = CreateWorkspace(xData,yData)
   run = input.getRun()
   run.addProperty("goodfrm","10","None",True)
   output,norm=CalculateMuonAsymmetry   (InputWorkspace=input,spectra=0,StartX=1,EndX=5,FittingFunction= "name = GausOsc, A = 10.0, Sigma = 0.2, Frequency = 1.0, Phi = 0.0",InputDataType="counts",Minimizer="Levenberg-MarquardtMD",MaxIterations=500 )
   print  "Asymmetry: ",['{0:.2f}'.format(value)  for value in output.readY(0)]
   print "Normalization constant: {0:.2f}".format(norm[0])

Output:

.. testoutput:: ExCounts

   Asymmetry:  ['10.00', '1.42', '-9.60', '-4.14', '8.42', '6.53', '-6.57', '-8.39', '4.20', '9.58', '-1.48', '-10.00', '-1.35', '9.62', '4.08', '-8.46', '-6.48', '6.62', '8.36', '-4.25', '-9.56', '1.54']
   Normalization constant: 5.00

**Example - Calculating Asymmetry From Estimated Asymmetry:**
This example is for calculating the Asymmetry from an estimate of the asymmetry.

.. testcode:: ExAsymm

   import math
   import numpy as np
   xData=np.linspace(start=0,stop=10,num=22)
   yData=[]
   tau =  2.1969811
   for x in xData:
       yData.append(50.*(1+10.*math.cos(3.*x))*math.exp(-x/tau))
   input = CreateWorkspace(xData,yData)
   run = input.getRun()
   run.addProperty("goodfrm","10","None",True)
   estAsymm,estNorm=CalculateMuonAsymmetry(InputWorkspace=input,spectra=0,StartX=1,EndX=5)
   output,norm=CalculateMuonAsymmetry(InputWorkspace=estAsymm,spectra=0,StartX=1,EndX=5,FittingFunction= "name = GausOsc, A = 10.0, Sigma = 0.2, Frequency = 1.0, Phi = 0.0",InputDataType="asymmetry",Minimizer="Levenberg-MarquardtMD",MaxIterations=500,PreviousNormalizationConstant=estNorm )
   print  "Asymmetry: ",['{0:.2f}'.format(value)  for value in output.readY(0)]
   print "Normalization constant: {0:.2f}".format(norm[0])

Output:

.. testoutput:: ExAsymm

   Asymmetry:  ['10.00', '1.42', '-9.60', '-4.14', '8.42', '6.53', '-6.57', '-8.39', '4.20', '9.58', '-1.48', '-10.00', '-1.35', '9.62', '4.08', '-8.46', '-6.48', '6.62', '8.36', '-4.25', '-9.56', '1.54']
   Normalization constant: 5.00

.. categories::

.. sourcelink::
