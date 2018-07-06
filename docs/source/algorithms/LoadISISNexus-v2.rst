.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads a Nexus file created from an ISIS instrument.

Data loaded from Nexus File
###########################

Not all of the nexus file is loaded. This section tells you what is loaded and where it goes in the workspace.

The nexus file must have ``raw_data_1`` as its main group and
contain a ``/isis_vms_compat`` group to be loaded.

The workspace data is loaded from ``raw_data_1/Detector_1``.

Instrument information is loaded from ``raw_data_1/Instrument`` if available in file, 
otherwise :ref:`instrument information <InstrumentDefinitionFile>` is read from a MantidInstall instrument directory.

Also the ``NSP1``, ``UDET``, ``SPEC``, ``HDR``, ``IRPB``, ``RRPB``, ``SPB`` and ``RSPB`` sections of
``raw_data_1/isis_vms_compat`` are read. The contents of ``isis_vms_compat`` are a legacy from an older ISIS format.


Here are some tables that show it in more detail:

+------------------------------+-------------------------------------------+-------------------------------------+
| Description of Data          | Found in Nexus file                       | Placed in Workspace (Workspace2D)   |
|                              | (within 'raw_data_1')                     |                                     |
+==============================+===========================================+=====================================+
| Monitor Data                 | within groups of Class NXMonitor          | Monitor histogram data (loaded      | 
|                              | (one monitor per group)                   | depending on prop. LoadMonitors)    |
+------------------------------+-------------------------------------------+-------------------------------------+
| Detector Data                | group ``Detector_1``                      | Histogram Data                      |
|                              | (all detectors in one group)              |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+
| Instrument                   | group ``Instrument``                      | Workspace instrument                |
+------------------------------+-------------------------------------------+-------------------------------------+ 
| Spectrum of each detector ID | ``NSP1``, ``UDET`` and ``SPEC``           | Spectra-Detector mapping            |
|                              | within ``isis_vms_compat``                |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+  
| Run                          | various places as shown below             | Run object                          |
+------------------------------+-------------------------------------------+-------------------------------------+
| Sample                       | ``SPB`` and ``RSPB`` within               | Sample Object                       | 
|                              | ``isis_vms_compat``                       |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+

Run Object
''''''''''
LoadISISNexus executes :ref:`algm-LoadNexusLogs` to load run logs from the Nexus ``runlog`` or some other appropriate group. 
It also loads the Nexus ``raw_data_1/periods/proton_charge`` group 
into the ``proton_charge_by_period`` property of the workspace run object.

Properties of the workspace :ref:`Run <Run>` object are loaded as follows:

+----------------+-------------------------------------------------+
| Nexus          | Workspace run object                            |
+================+=================================================+
| ``HDR``        | ``run_header`` (spaces inserted between fields) |
+----------------+-------------------------------------------------+
| ``title``      | ``run_title``                                   |
+----------------+-------------------------------------------------+
| ``start_time`` | ``run_start``                                   |
+----------------+-------------------------------------------------+
| ``end_time``   | ``run_end``                                     |
+----------------+-------------------------------------------------+
| (data)         | ``nspectra``                                    |
+----------------+-------------------------------------------------+
| (data)         | ``nchannels``                                   |
+----------------+-------------------------------------------------+
| (data)         | ``nperiods``                                    |
+----------------+-------------------------------------------------+
| ``IRPB[0]``    | ``dur``                                         |
+----------------+-------------------------------------------------+
| ``IRPB[1]``    | ``durunits``                                    |
+----------------+-------------------------------------------------+
| ``IRPB[2]``    | ``dur_freq``                                    |
+----------------+-------------------------------------------------+
| ``IRPB[3]``    | ``dmp``                                         |
+----------------+-------------------------------------------------+
| ``IRPB[4]``    | ``dmp_units``                                   |
+----------------+-------------------------------------------------+
| ``IRPB[5]``    | ``dmp_freq``                                    |
+----------------+-------------------------------------------------+
| ``IRPB[6]``    | ``freq``                                        |
+----------------+-------------------------------------------------+
| ``IRPB[7]``    | ``gd_prtn_chrg``                                |
+----------------+-------------------------------------------------+
| ``RRPB[9]``    | ``goodfrm``                                     |
+----------------+-------------------------------------------------+
| ``RRPB[10]``   | ``rawfrm``                                      |
+----------------+-------------------------------------------------+
| ``RRPB[11]``   | ``dur_wanted``                                  |
+----------------+-------------------------------------------------+
| ``RRPB[12]``   | ``dur_secs``                                    |
+----------------+-------------------------------------------------+
| ``RRPB[13]``   | ``mon_sum1``                                    |
+----------------+-------------------------------------------------+
| ``RRPB[14]``   | ``mon_sum2``                                    |
+----------------+-------------------------------------------------+
| ``RRPB[15]``   | ``mon_sum3``                                    |
+----------------+-------------------------------------------------+
| ``RRPB[21]``   | ``rb_proposal``                                 |
+----------------+-------------------------------------------------+

In the Nexus column, names of groups in capitals are in ``raw_data_1/isis_vms_compat`` and the other groups are in ``raw_data_1``.

(data) indicates that the number is got from the histogram data in an appropiate manner.

``IRPB`` and ``RRPB`` point to the same data. In ``IRPB``, the data is seen as 32-bit integer 
and in RRPB it is seen as 32-bit floating point (same 32 bits).
In all cases, integers are passed. The 32-bit floating point numbers are used only to store larger integers.

The other indices of ``IRPB`` and ``RRPB`` are not read.


Sample Object
'''''''''''''

Properties of the workspace sample object are loaded as follows:

+-------------+-------------------------+
| Nexus       | Workspace sample object |
+=============+=========================+
| ``SPB[2]``  | Geometry flag           |
+-------------+-------------------------+
| ``RSPB[3]`` | Thickness               |
+-------------+-------------------------+
| ``RSPB[4]`` | Height                  |
+-------------+-------------------------+
| ``RSPB[5]`` | Width                   |
+-------------+-------------------------+

Nexus groups are found in ``raw_data_1/isis_vms_compat``. Other indices of ``SPB`` & ``RSPB`` are not read.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load without any optional arguments:**

.. testcode:: ExLoadISISnexus

   # Load LOQ histogram dataset
   ws = LoadISISNexus('LOQ49886.nxs')

   print("The 1st x-value of the first spectrum is: {}".format(ws.readX(0)[0]))

Output:

.. testoutput:: ExLoadISISnexus

   The 1st x-value of the first spectrum is: 5.0

**Example - Using SpectrumMin and SpectrumMax:**

.. testcode:: ExLoadSpectrumMinMax

   # Load from LOQ data file spectrum 2 to 3.
   ws = LoadISISNexus('LOQ49886.nxs',SpectrumMin=2,SpectrumMax=3)

   print("The number of histograms (spectra) is: {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: ExLoadSpectrumMinMax

   The number of histograms (spectra) is: 2

**Example - Using EntryNumber:**

.. testcode:: ExLoadEntryNumber

   # Load first period of multiperiod POLREF data file
   ws = LoadISISNexus('POLREF00004699.nxs', EntryNumber=1)

   print("The number of histograms (spectra) is: {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: ExLoadEntryNumber

   The number of histograms (spectra) is: 246

.. categories::

.. sourcelink::
