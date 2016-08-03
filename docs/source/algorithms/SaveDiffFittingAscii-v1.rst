.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves a TableWorkspace containing Diffraction Fitting results in to an ASCII file in TBL format.
All of the columns must be typed as 'double' in order to successfully save it as a ASCII file.
A GroupWorkspace of TableWorkspace can also be saved where the run number and bank is provided
manually as a list in the correct order.



Limitations
###########

The Algorithm will fail if any of the columns is not of type 'double'.

Usage
-----

**Example - Save a TableWorkspace in Diffraction Fitting Table format**


.. testcleanup:: ExSaveDiffFittingAscii

    os.remove(savefile)

Output:

.. testoutput:: ExSaveDiffFittingAscii

    File Exists: True

.. categories::

.. sourcelink::
