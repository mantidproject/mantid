.. _using_modules:

=============
Using Modules 
=============

-  Functions allow blocks of code to be separated into reusable parts
   that can be called from a user's script. Modules take this idea one
   step further and allow the grouping of functions and code that all
   perform similar tasks into a library, or in Python speak, *module*.

-  A module has its definitions, the functions and code that make it up,
   within a separate file ending with a '.py' extension and this file is
   then *imported* into a user's current script. This is most easily
   demonstrated through an example. We will define a mathematics module
   called *mymath* and use this from a separate script.

.. code:: python

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

-  To use this module from a user script we need to tell the script
   about the module using the ``import`` statement. Once the definitions
   are imported we can then use the functions by calling them as
   ``module.functionname``. This syntax is to prevent name clashes.

.. code:: python

   ## My script using the math module ##
   import mymath  # Note no .py
    
   values = [2,4,6,8,10]
   print('Squares:')
   for v in values:
       print(mymath.square(v))
   print('Cubes:')
   for v in values:
       print(mymath.cube(v))

   print('Average: ' + str(mymath.average(values)))

-  Another variant of the import statement,
   ``import``\ *``module``*\ ``as``\ *``new-name``* can be used to have
   the module referred to under a different name, which can be useful
   for modules with long names

.. code:: python

   import mymath as mt

   print(mt.square(2))
   print(mt.square(3))

-  It is possible to avoid the ``module.functionname`` syntax by using
   an alternate version of the import statement,
   ``from``\ *``module``*\ ``import *``. The functions can then be used
   as if they were defined in the current file. This is **dangerous**
   however as you do not have any protection against name conflicts when
   using multiple modules.

Importing Modules from Other Locations
======================================

If the module lives within the same directory as the script that is
using it, then there is no problem. For system modules, these are
pre-loaded into pythons sys.path list.

For example the numpy module location is here

.. code:: python

   import numpy
   print(numpy.__file__)

There are several ways to make modules available for import.

Modifying the PYTHONPATH
------------------------

This environmental variable is used by python on startup to determine
the locations of any additional modules. You can extend it before
launching your python console. For example on linux:

| ``export PYTHONPATH=$PYTONPATH:{Path to mymodule directory}``
| ``python``

On windows, it would look like this

| ``SET PYTHONPATH=%PYTHONPATH%;{Path to mymodule directory}``
| ``python``

Appending to sys.path
---------------------

Another way to make modules available for import is to append their
directory paths onto *sys.path* within your python session.

.. code:: python

   python
   >>> import sys
   >>> sys.path.append({Path to mymodule directory})
   >>> import mymodule

Python's Standarad Library
--------------------------

-  Python comes with a large number of standard modules that offer a
   wealth of functionality. The documentation for these modules can be
   found at http://www.python.org.

-  They are used in exactly the same manner as user-defined modules
   using the ``import`` statement. Some examples of two useful modules
   are shown below.

datetime
~~~~~~~~

-  This module provides processing for date/time formats, reference:
   http://docs.python.org/2/library/datetime.html

-  Example usage:

.. testcode:: datetime1

   import datetime as dt
   format = '%Y-%m-%dT%H:%M:%S'
   t1 = dt.datetime.strptime('2008-10-12T14:45:52', format)
   print('Day ' + str(t1.day))
   print('Month ' + str(t1.month))
   print('Minute ' + str(t1.minute))
   print('Second ' + str(t1.second))

   # Define todays date and time
   t2 = dt.datetime.now()
   diff = t2 - t1

Gives the output:

.. testoutput:: datetime1

    Day 12
    Month 10
    Minute 45
    Second 52

os.path
~~~~~~~

-  The os.path module provides facilities for path manipulation in an OS
   independent manner, reference
   http://docs.python.org/2/library/os.path.html

-  Example usage: Open some files in a common directory

.. code:: python

   import os.path

   directory = 'C:/Users/Files'
   file1 = 'run1.txt'
   fullpath = os.path.join(directory, file1)  # Join the paths together in
                                              # the correct manner

   # print stuff about the path
   print(os.path.basename(fullpath))  # prints 'run1.txt'
   print(os.path.dirname(fullpath))  # prints 'C:\Users\Files'

   # A userful function is expanduser which can expand the '~' token to a
   # user's directory (Documents and Settings\username on WinXP  and 
   # /home/username on Linux/OSX)
   print(os.path.expanduser('~/test')) # prints /home/[MYUSERNAME]/test on
                                      # this machine where [MYUSERNAME] is
                                      # replaced with the login

Numpy Introduction
------------------

-  Python extension designed for fast numerical computation:
   http://numpy.scipy.org/
-  Numpy provides multidimensional array objects, including masked
   arrays and matrices
-  Numpy uses c-style arrays, which provide locality of reference for
   fast access
-  Numpy comes with a vast assortment of inbuilt mathematical functions
   which can operate on the ndarrays. These functions are implemented in
   c, and optimised to give good performance
-  Now available as standard within Mantid on all three platforms. Full
   tutorial: http://www.scipy.org/Tentative_NumPy_Tutorial.
-  To use Numpy in a script you must first import the module at the top
   of your script

.. code:: python

   import numpy

Numpy Arrays
~~~~~~~~~~~~

-  Python lists are flexible as they can store any type.

-  Iteration can be slow though as they are not designed for efficiency
   in this area.

-  Numpy arrays only store a single type and provide optimized
   operations on these arrays.

-  Arrays can be created from standard python lists

.. code:: python

   import numpy
   x = numpy.array([1.3, 4.5, 6.8, 9.0])

-  There is also a function, ``arange``, which is numpy's counterpart to
   ``range``, i.e. it creates an array from a start to and end with a
   given increment

.. code:: python

   x = numpy.arange(start=0.0, stop=10.0, step=1.0)

Numpy Functions
~~~~~~~~~~~~~~~

-  Numpy arrays carry attributes around with them. The most important
   ones are:

   -  ndim: The number of axes or rank of the array
   -  shape: A tuple containing the length in each dimension
   -  size: The total number of elements

.. testcode:: numpy1

   import numpy

   x = numpy.array([[1,2,3], [4,5,6], [7,8,9]]) # 3x3 matrix
   print(x.ndim) # Prints 2
   print(x.shape) # Prints (3L, 3L)
   print(x.size) # Prints 9

Gives the output:

.. testoutput:: numpy1

    2
    (3, 3)
    9

-  Can be used just like Python lists

   -  x[1] will access the second element
   -  x[-1] will access the last element

-  Arithmetic operations apply element wise

.. testcode:: numpy2

   import numpy
   a = numpy.array( [20, 30, 40, 50] ) 
   b = numpy.arange( 4 ) 
   c = a-b 
   print(c)

Gives the output:

.. testoutput:: numpy2

    [20 29 38 47]

Built-in Methods
~~~~~~~~~~~~~~~~

-  Many standard numerical functions are available as methods out of the
   box:

.. code:: python

   x = numpy.array([1,2,3,4,5])
   avg = x.mean()
   sum = x.sum()
   sx = numpy.sin(x)

-  A complete list is available at:
   http://docs.scipy.org/doc/numpy/reference/
