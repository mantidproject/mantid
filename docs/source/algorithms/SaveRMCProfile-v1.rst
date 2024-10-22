
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

**Example - SaveRMCProfile**

.. testcode:: SaveRMCProfileExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=range(0,3), UnitX="Angstrom")

   # Create a filename
   import os
   path = os.path.join(os.path.expanduser("~"), "SaveRMCProfile.fq")

   # Save as G(r) file
   SaveRMCProfile(InputWorkspace=ws, InputType="G(r)", Title="Data_title", Filename=path)

   # Check that the file exists
   print(os.path.isfile(path))

Output:

.. testoutput:: SaveRMCProfileExample

    True

.. testcleanup:: SaveRMCProfileExample

   DeleteWorkspace(ws)
   import os
   try:
       path = os.path.join(os.path.expanduser("~"), "SaveRMCProfile.fq")
       os.remove(path)
   except:
       pass

.. categories::

.. sourcelink::
