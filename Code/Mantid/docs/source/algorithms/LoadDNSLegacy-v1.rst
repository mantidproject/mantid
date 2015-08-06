.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

This algorithm loads a DNS legacy data file into a :ref:`Workspace2D <Workspace2D>`. Two workspaces will be created: 

-  raw data workspace with the given name. 
-  workspace with normalization data (monitor counts or experiment duration by user's choice). The normalization workspace is named same as the data workspace, but has suffix "_NORM". 

The loader rotates the detector bank in the position given in the data file. No operations on the neutron counts are performed. Sample logs are dublicated for both, data and normalization workspaces.

This algorithm only supports DNS instrument in its configuration before major upgrade. 

Usage
-----

**Example - Load a DNS legacy .d_dat file:**

.. code-block:: python

   # data file.
   datafile = 'dn134011vana.d_dat'

   # Load dataset
   ws = LoadDNSLegacy(datafile, Polarisation='x', Normalization='monitor')

   print "This workspace has", ws.getNumDims(), "dimensions and has", ws.getNumberHistograms(), "histograms."

Output:

   This workspace has 2 dimensions and has 24 histograms.

.. categories::

.. sourcelink::
