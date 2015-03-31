.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads a DNS legacy .d_dat data file into a :ref:`Workspace2D <Workspace2D>` with
the given name.

The loader rotates the detector bank in the position given in the data file.

This algorithm only supports DNS instrument in its configuration before major upgrade. 

Usage
-----

**Example - Load a DNS legacy .d_dat file:**

.. code-block:: python

   # data file.
   datafile = 'dn134011vana.d_dat'

   # Load dataset
   ws = LoadDNSLegacy(datafile)

   print "This workspace has", ws.getNumDims(), "dimensions and has", \
        ws.getNumberHistograms(), "histograms."

Output:

   This workspace has 2 dimensions and has 24 histograms.

.. categories::
