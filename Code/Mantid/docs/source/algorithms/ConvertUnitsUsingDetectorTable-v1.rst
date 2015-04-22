
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm functions in the same basic way as :ref:`algm-ConvertUnits` but 
instead of reading the geometric parameters from the instrument, it uses values 
that are specified in a `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`__

Restrictions on the input workspace
###################################

-  Naturally, the X values must have a unit set, and that unit must be
   known to the `Unit Factory <http://www.mantidproject.org/Units>`__.
-  Only histograms, not point data, can be handled at present.
-  The algorithm will also fail if the source-sample distance cannot be
   calculated (i.e. the :ref:`instrument <instrument>` has not been
   properly defined).

Available units
---------------

The units currently available to this algorithm are listed
`here <http://www.mantidproject.org/Units>`__, along with equations specifying exactly how the
conversions are done.

Usage
-----

**Example - Convert to wavelength using parameters from a table**

.. testcode:: ConvertUnitsUsingDetectorTableExample

   # Create a host workspace
   ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
   # Create a TableWorkspace to hold the detector parameters
   detpars = CreateEmptyTableWorkspace()
   detpars.addColumn("int", "spectra")
   detpars.addColumn("double", "l1")
   detpars.addColumn("double", "l2")
   detpars.addColumn("double", "twotheta")
   detpars.addColumn("double", "efixed")
   detpars.addColumn("int", "emode")

   # Add the parameters
   detpars.addRow([1, 10.0, 5.0, 90.0, 0.0, 0])

   wsOut = ConvertUnitsUsingDetectorTable(ws,Target="Wavelength",DetectorParameters=detpars)

   print "Input",  ws.readX(0)[ws.blocksize()-1]
   print "Output",  wsOut.readX(0)[wsOut.blocksize()-1]

Output:

.. testoutput:: ConvertUnitsUsingDetectorTableExample

   Input 19800.0
   Output 5.22196485301

.. categories::

