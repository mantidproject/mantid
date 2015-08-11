:orphan:

.. testcode:: mwTest_Numpy_Functions[4]

   import numpy
   
   x = numpy.array([[1,2,3], [4,5,6], [7,8,9]]) # 3x3 matrix
   print x.ndim # Prints 2
   print x.shape # Prints (3L, 3L)
   print x.size # Prints 9

.. testoutput:: mwTest_Numpy_Functions[4]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   2
   (3L, 3L)
   9


.. testsetup:: mwTest_Numpy_Functions[26]

   import numpy

.. testcode:: mwTest_Numpy_Functions[26]

   a = numpy.array( [20,30,40,50] ) 
   b = numpy.arange( 4 ) 
   c = a-b 
   #c => array([20, 29, 38, 47])


.. testsetup:: mwTest_Numpy_Functions[40]

   import numpy

.. testcode:: mwTest_Numpy_Functions[40]

   x = numpy.array([1,2,3,4,5])
   avg = x.mean()
   sum = x.sum()
   sx = numpy.sin(x)


