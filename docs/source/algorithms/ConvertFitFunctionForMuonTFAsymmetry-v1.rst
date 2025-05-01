.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be run in two modes. The first is construct, which takes a user fitting function :math:`f(t)` and it converts it to a TF normalisation function

.. math:: N_0[1+f(t)] + A\exp(-\lambda t)

where :math:`N_0` is the normalisation constant, :math:`A`  is fixed to zero by default and :math:`\lambda` is fixed to the Muon lifetime. The initial value for the normalisation constant is from the normalisation table.

The second mode is extract, if the TF normalisation function is given it will return the user function.

This algorithm works for both single and multi domain functions.

The algorithm takes an optional boolean parameter `CopyTies`, which when true will copy the ties present in the input function. By default it is set to true.

Usage
-----

**Example - Converting a function:**
This example is for converting a function.

.. testcode:: example

   import mantid.simpleapi as mantid
   #create a normalisation table
   tab = CreateEmptyTableWorkspace()
   tab.addColumn('double', 'norm')
   tab.addColumn('str', 'name')
   tab.addColumn('str', 'method')

   tab.addRow([1.,"Run;;Group;;a;;Asym;;#1","Estimate"])
   tab.addRow([2.,"Run;;Group;;b;;Asym;;#1","Estimate"])
   tab.addRow([3.,"Run;;Group;;c;;Asym;;#1","Estimate"])
   tab.addRow([4.,"Run;;Group;;d;;Asym;;#1","Estimate"])

   #create original function and workspace
   myFunc='name=LinearBackground,A0=3,A1=    4;name=LinearBackground,A0=0,A1=2;ties=(f0.A1=3, f0.A0=f1.A0)'
   ws = CreateWorkspace(DataX=[1,2,3,4,5,6,7],    DataY=[1,2,3,4,5,6],OutputWorkspace="Run; Group; a; Asym; #1")

   TFFunc = ConvertFitFunctionForMuonTFAsymmetry(InputFunction=myFunc, NormalizationTable=tab,
                                                 WorkspaceList=["Run; Group; a; Asym; #1"], Mode="Construct")

   # do a fit with new function
   fit =mantid.AlgorithmManager.create("Fit")
   fit.setProperty("Function",str(TFFunc))
   fit.setProperty("InputWorkspace",ws)
   fit.setProperty("Output",'fitWS')
   fit.execute()
   fittedFunc = fit.getPropertyValue("Function")

   returnFunc =    ConvertFitFunctionForMuonTFAsymmetry(InputFunction=str(fittedFunc),NormalizationTable=tab,
                                                        WorkspaceList=["Run; Group; a; Asym; #1"], Mode="Extract")
   # 0 iteration fit to get param table -> won't change function values
   fit_output =    Fit(Function=str(returnFunc),InputWorkspace=ws,MaxIterations=0,Output="return_params")

   paramTable = fit_output.OutputParameters  # table containing the optimal fit parameters

   if paramTable.column(1)[0] == paramTable.column(1)[2]:
       print("Constant tie has been preserved")
   else:
       print("Constant tie has not been preserved")

   if paramTable.column(1)[1] == 3.0:
      print("Fix has been preserved")
   else:
      print("Fix has not been preserved")

Output:

.. testoutput:: example

   Constant tie has been preserved
   Fix has been preserved

.. categories::

.. sourcelink::
