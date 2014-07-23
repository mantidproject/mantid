.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a 2D workspace that contains a single value and an optional
error value. This was traditionally used for doing binary operations between
a workspace and a single value. However, now that the Python access allows
the standard binary operations with workspaces & single numbers, the requirement
for this algorithm is almost gone.

One use case for which it will be required is when performing operations using only
the MantidPlot GUI point-and-click approach.

Usage
-----

**Workspace with single y value and zero error:**

.. testcode::

   five = CreateSingleValuedWorkspace(5)

   print "Number of histograms:",five.getNumberHistograms()
   print "Length of y array:",len(five.readY(0))
   print "Length of e array:",len(five.readE(0))
   print "Length of x array:",len(five.readX(0))

   print "y value:",five.readY(0)
   print "e value:",five.readE(0)

Output:

.. testoutput::

   Number of histograms: 1
   Length of y array: 1
   Length of e array: 1
   Length of x array: 1
   y value: [ 5.]
   e value: [ 0.]

**Workspace with single y and e value:**

.. testcode::

   five = CreateSingleValuedWorkspace(5, 0.1)

   print "Number of histograms:",five.getNumberHistograms()
   print "Length of y array:",len(five.readY(0))
   print "Length of e array:",len(five.readE(0))
   print "Length of x array:",len(five.readX(0))

   print "y value:",five.readY(0)
   print "e value:",five.readE(0)

Output:

.. testoutput::

   Number of histograms: 1
   Length of y array: 1
   Length of e array: 1
   Length of x array: 1
   y value: [ 5.]
   e value: [ 0.1]

.. categories::
