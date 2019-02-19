.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

This algorithm loads a DNS legacy data file into a :ref:`Workspace2D <Workspace2D>`. The loader rotates the detector bank
in the position given in the data file.

.. note::

   If the data file contains wrong neutron wavelength, the correct wavelength (in Angstrom) can be specified in the **Wavelength** field.
   Leave it 0 if you are not sure. In this case wavelength will be read from the data file.

**Output**

- For diffraction mode data (only one time channel) output is the :ref:`Workspace2D <Workspace2D>` with the X-axis in the wavelength units.

- For TOF data (more than one time channel) output is the :ref:`Workspace2D <Workspace2D>` with the X-axis in TOF units. The lower bin boundary for the channel :math:`i`, :math:`t_i` is calculated as :math:`t_i = t_1 + t_{delay} + i*\Delta t`, where :math:`\Delta t` is the channel width and :math:`t_1` is the time-of-flight from the source (chopper) to sample. Given in the data file channel width is scaled by the *channel_width_factor* which can be set in the :ref:`parameter file <InstrumentParameterFile>`.


.. note::

   Since zero time channel is not specified, the algorithm can roll the TOF data to get elastic peak at the right position.
   For this the **ElasticChannel** - channel number where the elastic peak is observed without correction - should be specified.
   For commissioning period, the algorithm ignores the elastic channel number given in the data file.

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

First row must contain the listed column headers, other rows contain coil currents for each polarisation. Rows with different currents for one polarisation are allowed. Columns are separated by tab symbols.

This algorithm only supports DNS instrument in its configuration with one detector bank (polarisation analysis).

Usage
-----

**Example 1 - Load DNS diffraction mode .d_dat file:**

.. testcode:: LoadDNSLegacyEx1

   # data file
   datafile = 'dn134011vana.d_dat'

   # Load dataset
   ws = LoadDNSLegacy(datafile, Normalization='monitor')

   print("This workspace has {} dimensions and has {} histograms.".format(ws.getNumDims(), ws.getNumberHistograms()))

**Output:**

.. testoutput:: LoadDNSLegacyEx1

   This workspace has 2 dimensions and has 24 histograms.


**Example 2 - Load DNS TOF mode .d_dat file and find the elastic channel:**

.. testcode:: LoadDNSLegacyEx2

   # data file
   datafile = 'dnstof.d_dat'

   # Load dataset
   ws = LoadDNSLegacy(datafile, Normalization='no')
   print("This workspace has {} dimensions and has {} histograms.".format(ws.getNumDims(), ws.getNumberHistograms()))

   # sum spectra over all detectors
   ws_sum = SumSpectra(ws)
   # perform fit
   # Warning: this will work only if elastic peak is stronger than the other peaks!
   peak_center, sigma = FitGaussian(ws_sum, 0)
   print("Elastic peak center is at {:.0f} microseconds and has sigma={:.0f}.".format(round(peak_center), round(sigma)))

   # calculate the elastic channel number
   channel_width = ws.getRun().getProperty("channel_width").value
   tof1 = ws.getRun().getProperty("TOF1").value
   t_delay = ws.getRun().getProperty("delay_time").value
   epp = round((peak_center - tof1 - t_delay)/channel_width)

   print("The channel width is {} microseconds.".format(channel_width))
   print("The elastic channel number is: {:.0f}.".format(epp))

**Output:**

.. testoutput:: LoadDNSLegacyEx2

   This workspace has 2 dimensions and has 24 histograms.
   Elastic peak center is at 3023 microseconds and has sigma=62.
   The channel width is 40.1 microseconds.
   The elastic channel number is: 65.


**Example 3 - Load DNS TOF mode .d_dat file and specify the elastic channel and wavelength:**

.. testcode:: LoadDNSLegacyEx3

   # data file
   datafile = 'dnstof.d_dat'

   # Load dataset
   ws = LoadDNSLegacy(datafile, ElasticChannel=65, Normalization='no', Wavelength=4.2)

   # let's check that the elastic peak is at the right position
   from scipy.constants import m_n, h

   l1 = 0.4     # distance from chopper to sample, m
   l2 = 0.85   # distance from sample to detector, m
   wavelength = ws.getRun().getProperty("wavelength").value   # neutron wavelength, Angstrom

   # neutron velocity
   velocity = h/(m_n*wavelength*1e-10)

   # calculate elastic TOF (total)
   tof2_elastic = 1e+06*l2/velocity
   tof1 = ws.getRun().getProperty("TOF1").value
   t_delay = ws.getRun().getProperty("delay_time").value
   tof_elastic = t_delay + tof1 + tof2_elastic
   print ("Calculated elastic TOF: {:.0f} microseconds".format(round(tof_elastic)))

   # get elastic TOF from file
   ws_sum = SumSpectra(ws)
   peak_center, sigma = FitGaussian(ws_sum, 0)
   print ("Elastic TOF in the workspace: {:.0f} microseconds".format(round(peak_center)))

   # compare difference to the channel width
   channel_width = ws.getRun().getProperty("channel_width").value
   print("Difference = {:.0f} microseconds < channel width = {} microseconds."
         .format(round(tof_elastic - peak_center), channel_width, round(sigma)))
   channel_width = ws.getRun().getProperty("channel_width").value

**Output:**

.. testoutput:: LoadDNSLegacyEx3

   Calculated elastic TOF: 1327 microseconds
   Elastic TOF in the workspace: 1299 microseconds
   Difference = 28 microseconds < channel width = 40.1 microseconds.

.. categories::

.. sourcelink::
