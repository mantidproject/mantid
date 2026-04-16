.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads the file given into a :py:obj:`Workspace2D <mantid.dataobjects.Workspace2D>` with the given name.
The file should be in the SPE format, which is described `here <https://www.ncnr.nist.gov/dave/documentation/dcs_mslice.pdf>`_.
The workspace will have X :py:obj:`Units <mantid.kernel.UnitFactoryImpl>` of ``Energy transfer``.
The other axis will be binned and have :py:obj:`Units <mantid.kernel.UnitFactoryImpl>` of either Q (momentum transfer) or degrees, depending on the label in the input file.
The workspace will be flagged as a distribution.

File Format
###########

The expected file format is detailed on the :ref:`algm-SaveSPE` page.

Usage
-----

**Example - Load an SPE file:**

.. testcode:: ExLoadSPE

   #run the loader by pointing it at the appropriate file.
   #If it's in your managed user directories there's no need for an absolute path
   ws1 = LoadSPE("Example.spe")

   print("Number of Spectra: {}".format(ws1.getNumberHistograms()))
   print("Number of Bins: {}".format(ws1.blocksize()))

Output:

.. testoutput:: ExLoadSPE

   Number of Spectra: 32
   Number of Bins: 195


.. categories::

.. sourcelink::
