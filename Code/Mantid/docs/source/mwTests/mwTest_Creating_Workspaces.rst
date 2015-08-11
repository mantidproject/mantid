:orphan:

.. testcode:: mwTest_Creating_Workspaces[14]

   def PyInit(self):
     # Creates a generic output property
     self.declareProperty(WorkspaceProperty(name="OutputWorkspace", 
                                            defaultValue="",
                                            direction=Direction.Output))
   
   def PyExec(self):
     # A workspace with 5 rows with 9 bins & 10 bin boundaries
     # (i.e. a histogram)
     output_ws = WorkspaceFactory.create("Workspace2D", NVectors=5,
                                           XLength=10, YLength=9)
     self.setProperty("OutputWorkspace", output_ws)


.. testsetup:: mwTest_Creating_Workspaces[34]

   #use another algorithm as a proxy for self
   self = AlgorithmManager.createUnmanaged("Rebin")
   self.initialize()
   self.setPropertyValue("OutputWorkspace","fake_name")

.. testcode:: mwTest_Creating_Workspaces[34]

   # ...snipped...
   # A workspace with 5 rows with 9 bins & 10 bin boundaries
   nrows = 5
   nbins = 9
   output_ws = WorkspaceFactory.create("Workspace2D", NVectors=nrows, 
                                     XLength=nbins+1, YLength=nbins)
   for i in range(nrows):
     xdata = output_ws.dataX(i)
     ydata = output_ws.dataY(i)
     edata = output_ws.dataE(i)
     for j in range(nbins):
       xdata[j] = j
       ydata[j] = i*i
       edata[j] = j
     # end for loop   
     # final bin boundary
     xdata[nbins] = nbins
   
   self.setProperty("OutputWorkspace", output_ws)


.. testcode:: mwTest_Creating_Workspaces[68]

   # ...snipped...
   # A workspace with 5 rows with 9 bins & 10 bin boundaries
   nrows = 5
   nbins = 9
   output_ws = WorkspaceFactory.create("Workspace2D", NVectors=nrows,
                                     XLength=nbins+1, YLength=nbins)
   
   # Copies meta-data and creates a single bin workspace with 5 rows
   second_ws = WorkspaceFactory.create(output_ws, NVectors=5,
                                     XLength=2,YLength=1)
   
   # Copies meta-data and creates a workspace with 1 rows that is 
   # the same length as the original in Y & X
   third_ws = WorkspaceFactory.create(output_ws, NVectors=1)


.. testsetup:: mwTest_Creating_Workspaces[92]

   import numpy 
   #use another algorithm as a proxy for self
   self = AlgorithmManager.createUnmanaged("Rebin")
   self.initialize()
   self.setPropertyValue("OutputWorkspace","fake_name")

.. testcode:: mwTest_Creating_Workspaces[92]

   # ...snipped...
   # A workspace with 5 rows with 9 bins & 10 bin boundaries
   nrows = 5
   nbins = 9
   output_ws = WorkspaceFactory.create("Workspace2D", NVectors=nrows,
                                     XLength=nbins+1, YLength=nbins)
   
   xdata = numpy.arange(float(nbins+1)) # filled with 0->9
   ydata = 100*numpy.arange(float(nbins))
   edata = numpy.sqrt(ydata) # filled with 0->sqrt(800)
   
   for i in range(nrows):
      output_ws.setX(i, xdata)
      output_ws.setY(i, ydata)
      output_ws.setE(i, edata)
   
   self.setProperty("OutputWorkspace", output_ws)


