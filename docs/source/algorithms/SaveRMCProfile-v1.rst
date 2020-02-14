
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves files consistent with `RMCProfile <http://www.rmcprofile.org/Main_Page/>`_.
The header of the file contains the number of data points in the file, the function type,
and a title for the data. The body of the file is of the form ``x y``.

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - SavePDFGui**

.. testcode:: SavePDFGuiExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=range(0,3), UnitX="Angstrom")

   # Create a filename
   import os
   path = os.path.join(os.path.expanduser("~"), "saveRMCProfile.fq")

   # Save as G(r) file
   SavePDFGui(ws, "G(r)", "Data_title, path)

   # Check that the file exists
   print(os.path.isfile(path))

Output:

.. testoutput:: SavePDFGuiExample

    True

.. testcleanup:: SavePDFGuiExample

   DeleteWorkspace(ws)
   import os
   try:
       path = os.path.join(os.path.expanduser("~"), "saveRMCProfile.fq")
       os.remove(path)
   except:
       pass

.. categories::

.. sourcelink::

