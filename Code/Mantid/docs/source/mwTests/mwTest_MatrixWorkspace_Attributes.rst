:orphan:

.. testsetup:: mwTest_MatrixWorkspace_Attributes[5]

   raw_workspace = Load("MAR11015")

.. testcode:: mwTest_MatrixWorkspace_Attributes[5]

   # Get the Y vector for the second row of data
   y_data2 = raw_workspace.readY(1)
   for y in y_data2:
       print y
   
   # Or in loop access. Print the first value in all spectra
   for index in range(0, raw_workspace.getNumberHistograms()):
       #Note the round brackets followed by the square brackets
       print raw_workspace.readY(index)[0]

.. testoutput:: mwTest_MatrixWorkspace_Attributes[5]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   24571.0
   13761.0
   ...
   62.0
   95.0


.. testsetup:: mwTest_MatrixWorkspace_Attributes[28]

   raw_workspace = Load("MAR11015")

.. testcode:: mwTest_MatrixWorkspace_Attributes[28]

   # raw_workspace from above
   print raw_workspace.getNumberHistograms()

.. testoutput:: mwTest_MatrixWorkspace_Attributes[28]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   922


.. testsetup:: mwTest_MatrixWorkspace_Attributes[41]

   raw_workspace = Load("MAR11015")

.. testcode:: mwTest_MatrixWorkspace_Attributes[41]

   # raw_workspace from above
   print raw_workspace.blocksize()

.. testoutput:: mwTest_MatrixWorkspace_Attributes[41]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   2663


.. testsetup:: mwTest_MatrixWorkspace_Attributes[59]

   workspace1= Load("MAR11015")
   workspace2= CloneWorkspace(workspace1)

.. testcode:: mwTest_MatrixWorkspace_Attributes[59]

   w1 = mtd['workspace1']
   w2 = mtd['workspace2']
   
   # Sum the two workspaces and place the output into a third
   w3 = w1 + w2
   
   # Multiply the new workspace by 2 and place the output into a new workspace
   w4 = w3 * 2


.. testsetup:: mwTest_MatrixWorkspace_Attributes[76]

   w1= Load("MAR11015")
   w2= CloneWorkspace(w1)

.. testcode:: mwTest_MatrixWorkspace_Attributes[76]

   # Multiply a workspace by 2 and replace w1 with the output
   w1 *= 2.0
   
   # Add 'workspace2' to 'workspace1' and replace 'workspace1' with the output
   w1 += w2


