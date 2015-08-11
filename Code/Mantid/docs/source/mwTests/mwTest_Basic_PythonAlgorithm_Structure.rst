:orphan:

.. testcode:: mwTest_Basic_PythonAlgorithm_Structure[4]

   from mantid.kernel import *
   from mantid.api import *
   
   class HelloWorld(PythonAlgorithm):
   
       def PyInit(self):
           # Declare properties
           pass
   
   
       def PyExec(self):
           # Run the algorithm
           pass
   
   # Register algorithm with Mantid
   AlgorithmFactory.subscribe(HelloWorld)


.. testcode:: mwTest_Basic_PythonAlgorithm_Structure[37]

   from mantid.kernel import *
   from mantid.api import *
   
   class HelloWorld(PythonAlgorithm):
   
       def category(self):
           return 'MyTools'
   
      # The rest is the same as above


.. testcode:: mwTest_Basic_PythonAlgorithm_Structure[54]

   from mantid.kernel import *
   from mantid.api import *
   
   class HelloWorld(PythonAlgorithm):
   
       def category(self):
           return 'Useful\Tools'
   
       # The rest is the same as above


