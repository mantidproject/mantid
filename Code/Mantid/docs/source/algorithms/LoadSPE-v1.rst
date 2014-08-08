.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads the file given into a :ref:`Workspace2D <Workspace2D>` with the given name. The file should be in the SPE format, which is described `here <Media:Spe_file_format.pdf>`_. The workspace will have X units of :ref:`Energy transfer <Unit Factory>`. The other axis will be binned and have units of either `Momentum transfer / Q <Unit Factory>`_ or degrees, depending on the label in the input file. The workspace will be flagged as a distribution.

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

   print "Number of Spectra:", ws1.getNumberHistograms()
   print "Number of Bins:", ws1.blocksize()

Output:

.. testoutput:: ExLoadSPE

   Number of Spectra: 32
   Number of Bins: 195


.. categories::
