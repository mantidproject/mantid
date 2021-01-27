
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

This is the algorithm that allows to create a :ref:`TableWorkspace <TableWorkspace>` logbook containing metadata specific
to the selected instrument and relevant technique, without the need to load NeXus data in Mantid.

There are three levels of entries that can be included in a logbook: default, optional, and custom. The default entries
are always included, the optional entries are included when header for that entry is given, through `OptionalHeaders`,
and custom entries are included when the full path in the NeXus file is provided through `CustomEntries`.
The NeXus paths for the default and optional entries are specified in the :ref:`Instrument Parameter File <InstrumentParameterFile>`
relevant to the instrument that the data comes from. The header for the custom entry can be specified through `CustomHeaders`,
or not at all. In that case, the name for the custom entry will be derived from its NeXus path.

The logbook can be stored as a CSV file and read outside of the Mantid using spreadsheet software, such as Microsoft Excel.

Usage
-----
.. include:: ../usagedata-note.txt

**Example - GenerateLogbook for ILL D7 rawdata**

.. testcode:: ExGenerateLogbook_ILL_D7

    GenerateLogbook(Directory='ILL/D7',
                    OutputWorkspace='d7_logbook', Facility='ILL', Instrument='D7',
                    NumorRange=[396990,396993], OptionalHeaders='TOF',
		    CustomEntries='entry0/acquisition_mode')
    print("Number of numors in the logbook: {}".format(len(mtd['d7_logbook'].column(0))))
    print("Number of headers in the logbook: {}".format(len(mtd['d7_logbook'].row(0))))

Output:

.. testoutput:: ExGenerateLogbook_ILL_D7

    Number of numors in the logbook: 2
    Number of headers in the logbook: 5

.. testcleanup:: ExGenerateLogbook_ILL_D7

   mtd.clear()

.. categories::

.. sourcelink::
