:orphan:

.. testcode:: mwTest_Extending_Mantid_With_Python_Exercise_2_Solution[1]

   # Extending Mantid With Python: Exercise 2
   #
   # The aim if this exercise is to write a small Python algorithm that is able to print to the results log the first <code>n</code> numbers of the Fibonacci series. The series starts with <code>0,1</code> and then the next term is the sum of the two previous terms.
   #
   # The algorithm should:
   #  - Define a property that sets the maximum number of terms to print. Single letters are not considered good property names so a good name would be something like <code>NTerms</code>
   #  - The property should have documentation that shows in the GUI describing what it is used for.
   #  - Validate that <code>NTerms</code> property is greater or equal to 1 when set in the GUI.
   #  - The log message for the value of each term should be at <code>notice</code> level and in the format: "Term 1 in the Fibonacci series is: 0"
   #  - Add log message at debug level that prints the value of the <code>NTerms</code> property after it has been retrieved.
   #
   # As an additional exercise in understanding errors:
   # - On execution, check that the value of <code>NTerms</code> is less than or equal to 1000. If it is not then raise a <code>RuntimeError</code>.
   
   from mantid.kernel import *
   from mantid.api import *
    
   class FibonacciExercise(PythonAlgorithm):
    
       def PyInit(self):
           self.declareProperty(name="NTerms",defaultValue=-1,validator=IntBoundedValidator(lower=0),doc="Number of terms to print") 
   
       def PyExec(self):
           nterms = self.getProperty("NTerms").value
           self.log().debug("NTerms = " + str(nterms))
   
           if nterms > 1000:
               raise RuntimeError("Number of terms greater than 1000")
   
           # 0 should print something else
           if nterms < 1:
               msg = "Number of terms selected less than 1"
           else:
               # First two terms
               prev_2, prev_1 = 0, 1
   
               msg = "Term 1 in the Fibonacci series is: " + str(prev_2) + "\n"
               msg += "Term 2 in the Fibonacci series is: " + str(prev_1) + "\n"
               count = 2
               while count < nterms:
                   count += 1
                   current = prev_2 + prev_1
                   msg += "Term " + str(count) + " in the Fibonacci series is: " + str(current) + "\n"
                   prev_2 = prev_1
                   prev_1 = current
           # End of else
   
           # Display whole log message in single shot
           self.log().notice(msg)
   
   # Register algorithm with mantid
   AlgorithmFactory.subscribe(FibonacciExercise)


