:orphan:

.. testcode:: mwTest_PythonAlgorithm_Progress_Reporting[7]

   def PyExec(self):
       endrange=100000
   
       # Create a Progress object that covers the progress
       # of the whole algorithm (start=0.0,end=1.0) and will
       # report a maximum of endrange times
       prog_reporter = Progress(self,start=0.0,end=1.0,
                                nreports=endrange)
       for i in range(0,endrange):
          prog_reporter.report("Processing")


.. testcode:: mwTest_PythonAlgorithm_Progress_Reporting[26]

   def PyExec(self):
       endrange=100000
   
       # Create a Progress object that covers the progress 
       # of the whole algorithm (start=0.0,end=1.0)
       # and will report a maximum of endrange times
       prog_reporter = Progress(self,start=0.0,end=1.0,
                       nreports=endrange)
       for i in range(0,endrange/2):
          prog_reporter.report("Processing half")
   
       # Move progress to end
       prog_reporter.report(endrange,"Done")


.. testcode:: mwTest_PythonAlgorithm_Progress_Reporting[46]

   def PyExec(self):
       endrange=100000
   
       # Create a Progress object that covers the progress
       # of the whole algorithm (start=0.0,end=1.0)
       # and will report a maximum of endrange times
       prog_reporter = Progress(self,start=0.0,end=1.0,
                       nreports=endrange)
       for i in range(0,endrange):
          if i % 5 == 0:
            prog_reporter.reportIncrement(5,"Processing")


