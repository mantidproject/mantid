.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

If workspaces in the WorkspaceList exist then it is removed from Mantid.

Usage
-----

**Example - Delete using a list of objects**

.. testcode::

   # A small test workspace, with sample_ws as the handle
   sample_ws = CreateSingleValuedWorkspace(5.0)
   sample_ws2 = CreateSingleValuedWorkspace(5.0)

   DeleteWorkspaces([sample_ws,sample_ws2])

   print "sample_ws exists in mantid:",("sample_ws" in mtd)
   print "sample_ws2 exists in mantid:",("sample_ws2" in mtd)

Output:

.. testoutput::

   sample_ws exists in mantid: False
   sample_ws2 exists in mantid: False

**Example - Delete using a string list**

.. testcode::

   # A small test workspace, with sample_ws as the handle
   CreateSingleValuedWorkspace(5.0, OutputWorkspace="single_value")
   CreateSingleValuedWorkspace(5.0, OutputWorkspace="single_value2")

   DeleteWorkspaces("single_value, single_value2")

   print "single_value exists in mantid:",("single_value" in mtd)
   print "single_value2 exists in mantid:",("single_value2" in mtd)

Output:

.. testoutput::

   single_value exists in mantid: False
   single_value2 exists in mantid: False

.. categories::

.. sourcelink::
