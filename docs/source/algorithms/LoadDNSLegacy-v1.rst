.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

This algorithm loads a DNS legacy data file into a :ref:`Workspace2D <Workspace2D>`. The loader rotates the detector bank in the position given in the data file.

**Output**

- For diffraction mode data (only one time channel) output is the :ref:`Workspace2D <Workspace2D>` with the X-axis in the wavelength units.

- For TOF data (more than one time channel) output is the :ref:`Workspace2D <Workspace2D>` with the X-axis in TOF units. The lower bin boundary for the channel :math:`i`, :math:`t_i` is calculated as :math:`t_i = t_1 + t_{delay} + i*\Delta t`, where :math:`\Delta t` is the channel width and :math:`t_1` is the time-of-flight from the source (chopper) to sample. Given in the data file channel width is scaled by the *channel_width_factor* which can be set in the :ref:`parameter file <InstrumentParameterFile>`.

**Normalization**

The **Normalization** option offers the following choices:

- **duration**: data in the output workspace will be divided by experiment duration. The *normalized* sample log will be set to *duration*.

- **monitor**: data in the output workspace will be divided by monitor counts. The *normalized* sample log will be set to *monitor*.

- **no**: no normalization will be performed, data will be loaded as is. The *normalized* sample log will be set to *no*.

**Polarisation**

Since polarisation is not specified in the DNS legacy files, coil currents table is required to lookup for the polarisation and set the *polarisation* sample log. The default coil currents are given as *x_currents*, *y_currents* and *z_currents* parameters in the :ref:`parameter file <InstrumentParameterFile>` for x, y, and z polarisations, respectively.

Alternatively, the text file with the coil currents table may be provided (optionally). The coil currents table is a text file containing the following table.

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

First row must contain the listed column headers, other rows contain coil currents for each polarisation. Rows with different currents for one polarisation are alowed. Columns are separated by tab symbols.

This algorithm only supports DNS instrument in its configuration with one detector bank (polarisation analysis).

Usage
-----

**Example - Load DNS legacy .d_dat files:**

.. code-block:: python

   # data file.
   datafiles = 'dn134011vana.d_dat,dnstof.d_dat'

   # Load dataset
   ws = LoadDNSLegacy(datafiles, Normalization='monitor')

   print("This workspace has {} dimensions and has {} histograms.".format(ws.getNumDims(), ws.getNumberHistograms()))

Output:

   This workspace has 2 dimensions and has 24 histograms.

.. categories::

.. sourcelink::
