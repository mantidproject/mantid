.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Compare two MDWorkspaces (`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ or
:ref:`MDHistoWorkspace <MDHistoWorkspace>`) to see if they are the same.
This is mostly meant for testing/debugging use by developers.

**What is compared**: The dimensions, as well as the signal and error
for each bin of each workspace will be compared.

`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ : the events in each box will
be compared if the *CheckEvents* option is checked. The events would
need to be in the same order to match.

Usage
-----

**Example - compare two MD workspaces:**

.. testcode:: ExCompareMDWorkspaces

   # create sample inelastic workspace for MARI instrument containing 1 at all spectra 
   ws1=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10',UnitX='DeltaE')
   AddSampleLog(ws1,'Ei','12.','Number')
   # create the copy of first workspace
   ws1a=ws1
   # create second workspace which has different signals   
   ws2=ws1*2;
   # Convert to MD
   mdWs1 =ConvertToMD(InputWorkspace=ws1,QDimensions='|Q|',QConversionScales='Q in A^-1',SplitInto='100,100',MaxRecursionDepth='1')
   mdWs1a=ConvertToMD(InputWorkspace=ws1a,QDimensions='|Q|',QConversionScales='Q in A^-1',SplitInto='100,100',MaxRecursionDepth='1')
   mdWs2=ConvertToMD(InputWorkspace=ws2,QDimensions='|Q|',QConversionScales='Q in A^-1',SplitInto='100,100',MaxRecursionDepth='1')
   # compare the workspaces
   comp_rez1=CompareMDWorkspaces(mdWs1,mdWs1a)
   comp_rez2=CompareMDWorkspaces(mdWs1,mdWs2)   

   # print comparison results
   print "Workspaces mdWs1 and mdWs1a are equal? : {0}  : Comparison result: {1}".format(comp_rez1[0],comp_rez1[1])
   print "Workspaces mdWs1 and mdWs2  are equal? : {0} : Comparison result: {1}".format(comp_rez2[0],comp_rez2[1])   



   
**Output:**

.. testoutput:: ExCompareMDWorkspaces

    Workspaces mdWs1 and mdWs1a are equal? : True  : Comparison result: Success!
    Workspaces mdWs1 and mdWs2  are equal? : False : Comparison result: Box signal does not match (18360 vs 36720)

.. categories::
