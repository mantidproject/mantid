.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the asymmetry from the first muon
spectra in a workspace.
in a workspace will be corrected.

The formula for calculating the asymmetry (from counts) is given by:

.. math:: \textrm{NewData} = (\textrm{OldData}\times e^\frac{t}{\tau})/(F N_0) - 1.0,

where :math:`\tau` is the muon lifetime (2.1969811e-6 seconds), :math:`F` is the number of good frames and :math:`N_0` is a
fitted normalisation constant. The normalisation is calculated by fitting to the normalised counts which is given by

.. math:: \textrm{normalisedCounts}=(\textrm{OldData}\times e^\frac{t}{\tau})/F

and the fitting function is given by

.. math:: N_0[1+f(t)]

and the renormalized data is transformed via the equation:

.. math:: \textrm{NewData} = (\textrm{NormalisedCounts}/(N_0) - 1.0. 

Usage
-----

**Example - Calculating Asymmetry:**
This example is for calculating the Asymmetry for a single data set.

.. testcode:: ExCounts

   import math
   import numpy as np

   def makeData(name,norm):
       xData=np.linspace(start=0,stop=10,num=200)
       yData=np.sin(5.2*xData)
       result = (1-yData )*norm
       ws= CreateWorkspace(DataX=xData, DataY=result,OutputWorkspace=name)
       return ws

   #create a normalisation table
   tab = CreateEmptyTableWorkspace()
   tab.addColumn('double', 'norm')
   tab.addColumn('str', 'name')
   tab.addColumn('str', 'method')

   tab.addRow([11.,"a","Estimate"])
   tab.addRow([22.,"b","Estimate"])
   
   ws= makeData("a",2.30)
   ws2= makeData("b",1.10)
   
   myFunc='name=GausOsc,$domains=i,Frequency=5.;'  

   TFFunc = ConvertFitFunctionForMuonTFAsymmetry(InputFunction=myFunc,NormalizationTable=tab,WorkspaceList=["a"],Mode="Construct")
   CalculateMuonAsymmetry(NormalizationTable=tab, unNormalizedWorkspaceList=["a"],
                          ReNormalizedWorkspaceList=["b"], InputFunction= str(TFFunc),
                          OutputFitWorkspace="fit_result",StartX=0.1,EndX=9.9)

   print("Normalization constant for b: {0:.2f}".format(tab.column(0)[1]))

Output:

.. testoutput:: ExCounts

   Normalization constant for b: 2.30

**Example - Calculating Asymmetry For multiple data sets:**
This example is for calculating the Asymmetry for multuiple data sets.

.. testcode:: ExAsymm

   import math
   import numpy as np

   def makeData(name,norm):
      xData=np.linspace(start=0,stop=10,num=200)
      yData=np.sin(5.2*xData)
      result = (1-yData )*norm
      ws= CreateWorkspace(DataX=xData, DataY=result,OutputWorkspace=name)
      return ws

   #create a normalisation table
   tab = CreateEmptyTableWorkspace()
   tab.addColumn('double', 'norm')
   tab.addColumn('str', 'name')
   tab.addColumn('str', 'method')

   tab.addRow([11.,"a","Estimate"])
   tab.addRow([22.,"b","Estimate"])
   tab.addRow([22.,"c","Estimate"])
   tab.addRow([22.,"d","Estimate"])

   #create original function and workspace
   myFunc='name=GausOsc,$domains=i,Frequency=5.;'
   myFunc2='name=GausOsc,$domains=i,Frequency=5.;'
   multiFunc='composite=MultiDomainFunction,NumDeriv=1;'+myFunc+myFunc2+'ties=(f0.Frequency=f1.Frequency)'

   ws= makeData("a",2.30)
   ws2= makeData("b",1.10)
   ws3= makeData("c",4.1)
   ws4= makeData("d",2.0)

   TFFunc = ConvertFitFunctionForMuonTFAsymmetry(InputFunction=multiFunc, NormalizationTable=tab,
                                                 WorkspaceList=["a","c"], Mode="Construct")

   CalculateMuonAsymmetry(NormalizationTable=tab, unNormalizedWorkspaceList=["a","c"],
                          ReNormalizedWorkspaceList=["b","d"], InputFunction= str(TFFunc),
                          OutputFitWorkspace="fit_result",StartX=0.1,EndX=9.9)

   print("Normalization constant for b: {0:.2f}".format(tab.column(0)[1]))
   print("Normalization constant for d: {0:.2f}".format(tab.column(0)[3]))

Output:

.. testoutput:: ExAsymm

   Normalization constant for b: 2.30
   Normalization constant for d: 4.10
   
.. categories::

.. sourcelink::
