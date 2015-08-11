:orphan:

.. testcode:: mwTest_Using_Modules[4]

   ## File: mymath.py ##
   def square(n):
       return n*n
   
   def cube(n):
       return n*n*n
   
   def average(values):
       nvals = len(values)
       sum = 0.0
       for v in values:
           sum += v 
       return float(sum)/nvals


.. testsetup:: mwTest_Using_Modules[22]

   modulestr = '''## File: mymath.py ##
   def square(n):
       return n*n
   
   def cube(n):
       return n*n*n
   
   def average(values):
       nvals = len(values)
       sum = 0.0
       for v in values:
           sum += v 
       return float(sum)/nvals'''
   
   with open('mymath.py', 'w') as outfile:
      outfile.write(modulestr )

.. testcode:: mwTest_Using_Modules[22]

   ## My script using the math module ##
   import mymath  # Note no .py
    
   values = [2,4,6,8,10]
   print 'Squares:'
   for v in values:
       print mymath.square(v)
   print 'Cubes:'
   for v in values:
       print mymath.cube(v)
   
   print 'Average: ' + str(mymath.average(values))

.. testcleanup:: mwTest_Using_Modules[22]

   import os
   os.remove('mymath.py')

.. testoutput:: mwTest_Using_Modules[22]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Squares:
   4
   16
   36
   64
   100
   Cubes:
   8
   64
   216
   512
   1000
   Average: 6.0


.. testsetup:: mwTest_Using_Modules[76]

   modulestr = '''## File: mymath.py ##
   def square(n):
       return n*n
   
   def cube(n):
       return n*n*n
   
   def average(values):
       nvals = len(values)
       sum = 0.0
       for v in values:
           sum += v 
       return float(sum)/nvals'''
   
   with open('mymath.py', 'w') as outfile:
      outfile.write(modulestr )

.. testcode:: mwTest_Using_Modules[76]

   import mymath as mt
   
   print mt.square(2)
   print mt.square(3)

.. testcleanup:: mwTest_Using_Modules[76]

   import os
   os.remove('mymath.py')

.. testoutput:: mwTest_Using_Modules[76]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   4
   9


.. testcode:: mwTest_Using_Modules[115]

   import numpy
   print numpy.__file__

.. testoutput:: mwTest_Using_Modules[115]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ...


.. Skipping Test  mwTest_Using_Modules[135]


