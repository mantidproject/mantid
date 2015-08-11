:orphan:

.. testcode:: mwTest_PythonAlgorithm_Logging[14]

   def PyExec(self):
       param = self.getProperty("Inputvalue")
   
       self.log().information("The cube of the input value " + 
                              str(param) + " is " + str(i*i*i))


.. testcode:: mwTest_PythonAlgorithm_Logging[27]

   def PyExec(self):
       limit = self.getProperty('LoopLimit')
       sum = 0
       msg = ""
       for i in range(1, limit+1):
           sum += i*i
           msg += str(i*i) + ' '
       
       # This will be much more efficient, 
       # especially if LoopLimit is large
       self.log().information(msg)


