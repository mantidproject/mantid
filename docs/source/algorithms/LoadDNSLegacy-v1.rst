.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

This algorithm loads a DNS legacy data file into a :ref:`Workspace2D <Workspace2D>`. The loader rotates the detector bank in the position given in the data file. 

**Normalization**

The **Normalization** option offers the following choices:

- **duration**: data in the output workspace will be divided by experiment duration. The *normalized* sample log will be set to *duration*.

- **monitor**: data in the output workspace will be divided by monitor counts. The *normalized* sample log will be set to *monitor*.

- **no**: no normalization will be performed, data will be loaded as is. The *normalized* sample log will be set to *no*.

**Polarisation**

Since polarisation is not specified in the DNS legacy files, coil currents table is required to lookup for the polarisation and set the *polarisation* sample log. The coil currents table is a text file containing the following table.

+--------------+----------+-------+-------+-------+-------+
| polarisation | comment  |  C_a  |  C_b  |  C_c  |  C_z  |
+==============+==========+=======+=======+=======+=======+
|      x       |    7     |   0   |  -2   | -0.77 |  2.21 |          
+--------------+----------+-------+-------+-------+-------+
|      y       |    7     |   0   |  1.6  | -2.77 |  2.21 |          
+--------------+----------+-------+-------+-------+-------+
|      z       |    7     |   0   | 0.11  | -0.50 |   0   |          
+--------------+----------+-------+-------+-------+-------+
|      x       |    7     |   0   | -2.1  | -0.97 |  2.21 |          
+--------------+----------+-------+-------+-------+-------+

First row must contain the listed column headers, other rows contain coil currents for each polarisation. Rows with different currents for one polarisation are alowed. Columns are separated by tab symbols. This table must be provided to the user by instrument scientist.  

This algorithm only supports DNS instrument in its configuration before major upgrade. 

Usage
-----

**Example - Load a DNS legacy .d_dat file:**

.. code-block:: python

   # data file.
   datafile = 'dn134011vana.d_dat'
   coilcurrents = 'currents.txt'

   # Load dataset
   ws = LoadDNSLegacy(datafile, Normalization='monitor', CoilCurrentsTable=coilcurrents)

   print "This workspace has", ws.getNumDims(), "dimensions and has", ws.getNumberHistograms(), "histograms."

Output:

   This workspace has 2 dimensions and has 24 histograms.

.. categories::

.. sourcelink::
