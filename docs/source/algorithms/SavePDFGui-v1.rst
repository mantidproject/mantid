
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves G(r) files consistent with `PDFGui <http://www.diffpy.org/>`_.
The body of the file is of the form ``r Gr dr dGr``.

Usage
-----

**Example - SavePDFGui**

.. testcode:: SavePDFGuiExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=range(0,3), UnitX="Angstrom")

   # Create a filename
   import os
   path = os.path.join(os.path.expanduser("~"), "savepdfgui.gr")

   # Save as G(r) file
   SavePDFGui(ws, path)

   # Check that the file exists
   print(os.path.isfile(path))

Output:

.. testoutput:: SavePDFGuiExample

    True

.. testcleanup:: SavePDFGuiExample

   DeleteWorkspace(ws)
   import os
   try:
       path = os.path.join(os.path.expanduser("~"), "savepdfgui.gr")
       os.remove(path)
   except:
       pass

.. categories::

.. sourcelink::
