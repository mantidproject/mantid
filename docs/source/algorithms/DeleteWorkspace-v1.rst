.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

If the input workspace exists then it is removed from Mantid.

Usage
-----

**Example - Delete using a handle**

.. testcode::

   # A small test workspace, with sample_ws as the handle
   sample_ws = CreateSingleValuedWorkspace(5.0)

   DeleteWorkspace(sample_ws)

   print("sample_ws exists in mantid: {}".format("sample_ws" in mtd))

Output:

.. testoutput::

   sample_ws exists in mantid: False

**Example - Delete using a string**

.. testcode::

   # A small test workspace, with sample_ws as the handle
   CreateSingleValuedWorkspace(5.0, OutputWorkspace="single_value")

   DeleteWorkspace("single_value")

   print("single_value exists in mantid: {}".format("single_value" in mtd))

Output:

.. testoutput::

   single_value exists in mantid: False

.. categories::

.. sourcelink::
