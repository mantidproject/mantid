
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves S(Q)-1 files consistent with `RMCProfile <http://www.rmcprofile.org/Main_Page>`_.
The body of the file is of the form ``Q SQ-1 dQ dSQ``.

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - SaveRMCProfile**

.. testcode:: SaveRMCProfileExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=range(0,3), UnitX="Angstrom^-1")

   # Create a filename
   import os
   path = os.path.join(os.path.expanduser("~"), "savermcprofile.gr")

   # Save as S(Q)-1 file
   SavePDFGui(ws, path)

   # Check that the file exists
   print(os.path.isfile(path))

Output:

.. testoutput:: SaveRMCProfileExample

    True

.. testcleanup:: SaveRMCProfileExample

   DeleteWorkspace(ws)
   import os
   try:
       path = os.path.join(os.path.expanduser("~"), "savermcprofile.gr")
       os.remove(path)
   except:
       pass

.. categories::

.. sourcelink::

