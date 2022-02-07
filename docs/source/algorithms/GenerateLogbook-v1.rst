
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

This is the algorithm that allows to create a :ref:`TableWorkspace <Table Workspaces>` logbook containing metadata specific
to the selected instrument and relevant technique, without the need to load NeXus data in Mantid.

There are three levels of entries that can be included in a logbook: default, optional, and custom. The default entries
are always included, the optional entries are included when header for that entry is given, through `OptionalHeaders`,
and custom entries are included when the full path in the NeXus file is provided through `CustomEntries`.
The NeXus paths for the default and optional entries are specified in the :ref:`Instrument Parameter File <InstrumentParameterFile>`
relevant to the instrument that the data comes from. The header for the custom entry can be specified through `CustomHeaders`,
or not at all. In that case, the name for the custom entry will be derived from its NeXus path.

The files in the directory are expected to be uniquely raw data for the chosen instrument and facility, specified through
`Facility` and `Instrument`, respectively. The range of files to be logged can be specified through `NumorRange` property,
where both edges of the range are inclusive.

Basic binary arithmetic operations: addition (the only one supported if the data in the entry is a string), subtraction,
multiplication and division on entries data are supported. No support is offered for organizing the operations using parentheses
or scaling by read data by constant numbers.

Binary operations can also be defined for entries that are not always present in the data, for example in the case of D7 at the ILL,
NeXus files contain a varying number of entries, from 1 to 6. There, by defining a sum over all possible entries containing polarisation
identifier, it is possible to get to display in the logbook which polarisations are actually contained in the data file.

The logbook can be stored as a CSV file and read outside of the Mantid using spreadsheet software, such as Microsoft Excel.

Usage
-----
.. include:: ../usagedata-note.txt

**Example - GenerateLogbook for ILL D7 rawdata**

.. testcode:: ExGenerateLogbook_D7

   data_dirs = config['datasearch.directories'].split(';')
   unit_test_data_dir = [p for p in data_dirs if 'UnitTest' in p][0]
   data_directory = os.path.abspath(os.path.join(unit_test_data_dir, 'ILL/D7'))

   GenerateLogbook(Directory=data_directory,
                   OutputWorkspace='d7_logbook', Facility='ILL', Instrument='D7',
                   NumorRange=[396990,396991,396993], OptionalHeaders='TOF',
                   CustomEntries='entry0/acquisition_mode')
   print("Number of numors in the logbook: {}".format(len(mtd['d7_logbook'].column(0))))
   print("Number of headers in the logbook: {}".format(len(mtd['d7_logbook'].row(0))))

Output:

.. testoutput:: ExGenerateLogbook_D7

   Number of numors in the logbook: 3
   Number of headers in the logbook: 9

.. testcleanup:: ExGenerateLogbook_D7

   mtd.clear()

**Example - GenerateLogbook for ILL D7 rawdata with binary operations**

.. testcode:: ExGenerateLogbook_D7_binary_operations

   data_dirs = config['datasearch.directories'].split(';')
   unit_test_data_dir = [p for p in data_dirs if 'UnitTest' in p][0]
   data_directory = os.path.abspath(os.path.join(unit_test_data_dir, 'ILL/D7'))

   GenerateLogbook(Directory=data_directory,
                   OutputWorkspace='d7_logbook', Facility='ILL', Instrument='D7',
                   NumorRange="396990:396993", CustomHeaders='wavelength',
                   CustomEntries='/entry0/D7/POL/actual_state+/entry0/D7/POL/actual_stateB1B2')
   print("Number of numors in the logbook: {}".format(len(mtd['d7_logbook'].column(0))))
   print("Number of headers in the logbook: {}".format(len(mtd['d7_logbook'].row(0))))

Output:

.. testoutput:: ExGenerateLogbook_D7_binary_operations

   Number of numors in the logbook: 3
   Number of headers in the logbook: 8

.. testcleanup:: ExGenerateLogbook_D7_binary_operations

   mtd.clear()

.. categories::

.. sourcelink::
