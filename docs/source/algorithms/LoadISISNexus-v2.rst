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

The nexus file must have a ``raw_data_1`` top-level entry to be loaded.

The workspace data is loaded from ``raw_data_1/Detector_1``.

Instrument information is loaded from ``raw_data_1/Instrument`` if available in file,
otherwise :ref:`instrument information <InstrumentDefinitionFile>` is read from a MantidInstall instrument directory.

If the main entry contains a ``/isis_vms_compat`` the following entries will be read by the Algorithm:
``NSP1``, ``UDET``, ``SPEC``, ``HDR``, ``IRPB``, ``RRPB``, ``SPB`` and ``RSPB``.
The contents of ``isis_vms_compat`` are a legacy from an older ISIS format. If the legacy vms_compat block is not
present, an equivalent entry in the file is used.

In the tables below, descriptions of the relevant nexus entries are given. If applicable, the location within the
vms_block and an alternative location to read the entry from is given. In these tables ``/group`` refers to a group under
the main entry, i.e ``/group`` is equivalent to ``raw_data_1/group``.

+------------------------------+-------------------------------------------+-------------------------------------+
| Description of entry         | vms_compat entry                          | Alternative Location                |
+==============================+===========================================+=====================================+
| Spectra-Detector mapping     | ``NSP1``, ``UDET`` and ``SPEC``           | /detector_1/spectrum_index          |
|                              | within ``isis_vms_compat``                | (assumes 1-1 mapping)               |
+------------------------------+-------------------------------------------+-------------------------------------+
| Sample                       | ``SPB`` and ``RSPB`` within               | /sample group                       |
|                              | ``isis_vms_compat``                       |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+

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
| Run                          | various places as shown below             | Run object                          |
+------------------------------+-------------------------------------------+-------------------------------------+

Run Object
''''''''''
LoadISISNexus executes :ref:`algm-LoadNexusLogs` to load run logs from the Nexus ``runlog`` or some other appropriate group.
It also loads the Nexus ``raw_data_1/periods/proton_charge`` group
into the ``proton_charge_by_period`` property of the workspace run object.

Properties of the workspace :ref:`Run <Run>` object are loaded as follows:

+----------------+---------------------------------+-----------------------+
| vms_compat     | Alternative location            | Workspace run object  |
+================+=================================+=======================+
| ``title``      |  ``/title``                     | ``run_title``         |
+----------------+---------------------------------+-----------------------+
| ``start_time`` | ``/start_time``                 | ``run_start``         |
+----------------+---------------------------------+-----------------------+
| ``end_time``   | ``/end_time``                   | ``run_end``           |
+----------------+---------------------------------+-----------------------+
| (data)         | (data)                          | ``nspectra``          |
+----------------+---------------------------------+-----------------------+
| (data)         | (data)                          | ``nchannels``         |
+----------------+---------------------------------+-----------------------+
| (data)         | (data)                          | ``nperiods``          |
+----------------+---------------------------------+-----------------------+
| ``IRPB[6]``    | ``/instrument/source/frequency``| ``freq``              |
+----------------+---------------------------------+-----------------------+
| ``IRPB[7]``    |``/proton_charge``               | ``gd_prtn_chrg``      |
+----------------+---------------------------------+-----------------------+
| ``RRPB[9]``    |``good_frames``                  | ``goodfrm``           |
+----------------+---------------------------------+-----------------------+
| ``RRPB[10]``   |``/raw_frames``                  |  ``rawfrm``           |
+----------------+---------------------------------+-----------------------+
| ``RRPB[21]``   |``/experiment_identifier``       |  ``rb_proposal``      |
+----------------+---------------------------------+-----------------------+

The group (data) indicates that the number is obtained from the histogram data in an appropriate manner.

``IRPB`` and ``RRPB`` point to the same data. In ``IRPB``, the data is seen as 32-bit integer
and in RRPB it is seen as 32-bit floating point (same 32 bits).
In all cases, integers are passed. The 32-bit floating point numbers are used only to store larger integers.

The other indices of ``IRPB`` and ``RRPB`` are not read.


Sample Object
'''''''''''''

Properties of the workspace sample object are loaded as follows:

+-------------+-------------------------+-------------------------+
| vms_compat  | Alternative location    | Workspace sample object |
+=============+=========================+=========================+
| ``SPB[2]``  | ``/sample/id``          | Geometry flag           |
+-------------+-------------------------+-------------------------+
| ``RSPB[3]`` | ``/sample/thickness``   | Thickness               |
+-------------+-------------------------+-------------------------+
| ``RSPB[4]`` | ``/sample/height``      | Height                  |
+-------------+-------------------------+-------------------------+
| ``RSPB[5]`` | ``/sample/width``       | Width                   |
+-------------+-------------------------+-------------------------+

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
