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

**Example - Calculating Asymmetry for double pulse data:**

.. testcode:: AsymmDoublePulse

   import math
   import numpy as np

   delta = 0.33
   x = np.linspace(0.,15.,100)
   x_offset = np.linspace(delta/2, 15. + delta/2, 100)
   x_offset_neg = np.linspace(-delta/2, 15. - delta/2, 100)

   testFunction = GausOsc(Frequency = 1.5, A=0.22)
   y1 = testFunction(x_offset_neg)
   y2 = testFunction(x_offset)
   N0 = 6.38
   y = N0 * (1 + y1/2+y2/2)
   y_norm = y1/2+y2/2
   unnormalised_workspace = CreateWorkspace(x,y)
   ws_to_normalise = CreateWorkspace(x,y)
   ws_correctly_normalised = CreateWorkspace(x,y_norm)
   AddSampleLog(Workspace='ws_to_normalise', LogName="analysis_asymmetry_norm", LogText="1")

   innerFunction = FunctionFactory.createInitialized('name=GausOsc,A=0.20,Sigma=0.2,Frequency=1.0,Phi=0')
   tf_function = ConvertFitFunctionForMuonTFAsymmetry(InputFunction=innerFunction, WorkspaceList=['ws_to_normalise'])

   CalculateMuonAsymmetry(MaxIterations=100, EnableDoublePulse=True, PulseOffset=delta, UnNormalizedWorkspaceList='unnormalised_workspace', ReNormalizedWorkspaceList='ws_to_normalise',
                           OutputFitWorkspace='DoublePulseFit', StartX=0, InputFunction=str(tf_function), Minimizer='Levenberg-Marquardt')

   double_parameter_workspace = AnalysisDataService.retrieve('DoublePulseFit_Parameters')
   values_column = double_parameter_workspace.column(1)

   print("Normalization constant is: {0:.2f}".format(values_column[0]))

Output:

.. testoutput:: AsymmDoublePulse

   Normalization constant is: 6.38

.. categories::

.. sourcelink::
