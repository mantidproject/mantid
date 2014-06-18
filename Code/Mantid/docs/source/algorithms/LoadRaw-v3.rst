.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadRaw algorithm stores data from the `RAW <RAW_File>`__ file in a
`Workspace2D <Workspace2D>`__, which will naturally contain histogram
data with each spectrum going into a separate histogram. The time bin
boundaries (X values) will be common to all histograms and will have
their `units <units>`__ set to time-of-flight. The Y values will contain
the counts and will be unit-less (i.e. no division by bin width or
normalisation of any kind). The errors, currently assumed Gaussian, will
be set to be the square root of the number of counts in the bin.

Optional properties
###################

If only a portion of the data in the `RAW <RAW_File>`__ file is
required, then the optional 'spectrum' properties can be set before
execution of the algorithm. Prior to loading of the data the values
provided are checked and the algorithm will fail if they are found to be
outside the limits of the dataset.

Multiperiod data
################

If the RAW file contains multiple periods of data this will be detected
and the different periods will be output as separate workspaces, which
after the first one will have the period number appended (e.g.
OutputWorkspace\_period). Each workspace will share the same
`Instrument <Instrument>`__, SpectraToDetectorMap and
`Sample <Sample>`__ objects. If the optional 'spectrum' properties are
set for a multiperiod dataset, then they will be ignored.

If PeriodList property isn't empty then only periods listed there will be
loaded.

Subalgorithms used
##################

LoadRaw runs the following algorithms as child algorithms to populate
aspects of the output `Workspace <Workspace>`__:

-  :ref:`algm-LoadInstrument` - Looks for an instrument
   definition file named XXX\_Definition.xml, where XXX is the 3 letter
   instrument prefix on the RAW filename, in the directory specified by
   the "instrumentDefinition.directory" property given in the config
   file (or, if not provided, in the relative path ../Instrument/). If
   the instrument definition file is not found then the
   :ref:`algm-LoadInstrumentFromRaw` algorithm will be
   run instead.
-  :ref:`algm-LoadMappingTable` - To build up the mapping
   between the spectrum numbers and the Detectors of the attached
   `Instrument <Instrument>`__.
-  :ref:`algm-LoadLog` - Will look for any log files in the same
   directory as the RAW file and load their data into the workspace's
   `Sample <Sample>`__ object.

Previous Versions
-----------------

LoadRaw version 1 and 2 are no longer available in Mantid. Version 3 has
been validated and in active use for several years, if you really need a
previous version of this algorithm you will need to use an earlier
version of Mantid.

.. categories::
