.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates a 2D workspace that contains a single value and an optional
error value. This was traditionally used for doing binary operations between
a workspace and a single value. However, now that the Python access allows
the standard binary operations with workspaces & single numbers, the requirement
for this algorithm is almost gone and is occasionally used internally.

Usage
-----

**Workspace with single y value and zero error:**

.. testcode::

   five = CreateSingleValuedWorkspace(5)

   print("Number of histograms: {}".format(five.getNumberHistograms()))
   print("Length of y array: {}".format(len(five.readY(0))))
   print("Length of e array: {}".format(len(five.readE(0))))
   print("Length of x array: {}".format(len(five.readX(0))))

   print("y value: {}".format(five.readY(0)))
   print("e value: {}".format(five.readE(0)))

Output:

.. testoutput::

   Number of histograms: 1
   Length of y array: 1
   Length of e array: 1
   Length of x array: 1
   y value: [5.]
   e value: [0.]

**Workspace with single y and e value:**

.. testcode::

   five = CreateSingleValuedWorkspace(5, 0.1)

   print("Number of histograms: {}".format(five.getNumberHistograms()))
   print("Length of y array: {}".format(len(five.readY(0))))
   print("Length of e array: {}".format(len(five.readE(0))))
   print("Length of x array: {}".format(len(five.readX(0))))

   print("y value: {}".format(five.readY(0)))
   print("e value: {}".format(five.readE(0)))

Output:

.. testoutput::

   Number of histograms: 1
   Length of y array: 1
   Length of e array: 1
   Length of x array: 1
   y value: [5.]
   e value: [0.1]

.. categories::

.. sourcelink::
