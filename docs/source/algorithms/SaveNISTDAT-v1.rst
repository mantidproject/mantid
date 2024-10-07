.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Qxy rebins a 2D workspace in units of wavelength into 2D Q. It also
normalises to the solid angle of each detector pixel. The result is
stored in a 2D workspace with two numeric axes, both in units of Q.
SaveNISTDAT can then save the output of Qxy to an ASCII file in a
form that can be read by NIST software.

Usage
-----

**Example - Saving Some Pre-existing Data**

.. include:: ../usagedata-note.txt

.. testcode:: ExSaveRoundtrip

   import os

   # Load in some data which is already in a form that SaveNISTDAT expects,
   # then save it back out to a file.
   data = Load("saveNISTDAT_data.nxs")
   file_path = os.path.join(config["defaultsave.directory"], "example.dat")
   SaveNISTDAT(data, file_path)

   # Load it back in and inspect what we have.
   reloaded_data = LoadAscii(file_path)
   print("The data read back in is " + str(reloaded_data.readY(0)))
   # N.B. Mantid does not properly support reloading files output from 'SaveNISTDAT'
   # Above, the reloaded file is vastly altered from the original
   # due to the conversion performed upon running 'SaveNISTDAT'.
   # To see this, 'Show Data' on the data and reloaded_data workspaces.

.. testcleanup:: ExSaveRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSaveRoundtrip

   The data read back in is [-0.0735 -0.0735 -0.0735 ...  0.0685  0.0685  0.0685]

.. categories::

.. sourcelink::
