.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads an ILL TOF NeXus file into a :ref:`Workspace2D <Workspace2D>` with the given name.

To date this algorithm only supports: IN4, IN5, IN6, PANTHER, and SHARP.

By default, this algorithm loads the data indexed by channels. To convert to time-of-flight, use the `ConvertToTOF` option.

This algorithm also supports diffraction mode. In this case, the unit of the output workspace will be wavelength instead of time-of-flight or channel.

The support for the omega scan measurement mode for IN5, PANTHER, and SHARP is also handled by this loader, with the assumption that only one monitor exists. The X-axis
unit is `Label` with values provided by the scanned parameter. In this case, the setting to convert to the time-of-flight with `ConvertToTOF` has no effect.

.. note::
    The initial time-of-flight axis is set up using the 'time_of_flight' field in the NeXus file. Therefore the conversion from 'TOF' to 'DeltaE' may not give the correct zero-energy transfer.

.. note::
    LoadILLTOF3 uses HDF5 and cannot read former HDF4-based NeXus files. Please use LoadILLTOF2 in that case.

Pulse Intervals
---------------

For IN4 and IN6 the algorithm also calculates the pulse interval.

For the number of pulses:

* **IN4:** :math:`n_{pulses} = \frac{v_{fc}}{4 v_{bc}}`
    where :math:`n_{pulses}` is the number of pulses from the chopper per rotation, :math:`v_{fc}` the Fermi chopper speed and :math:`v_{bc}` the background chopper speed. Background chopper 1 and background chopper 2 must have the same speeds. All speeds are in units of rpm.

* **IN6:** :math:`n_{pulses} = \frac{v_{fc}}{v_{sc}}`
    where :math:`n_{pulses}` is the number of pulses from the chopper per rotation, :math:`v_{fc}` the Fermi chopper speed and :math:`v_{sc}` the suppressor chopper speed. All speeds are in units of rpm.

The pulse interval, :math:`T_{pulse}` in seconds, is then given by,

:math:`T_{pulse} = \frac{60 \textrm{s}}{2 v_{fc}} n_{pulses}`.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load a regular histogram NeXus file:**

.. testcode:: ExLoad

    # Regular data file.
    dataRegular = 'ILL/IN5/104007.nxs'

    # Load ILL dataset
    ws = Load(dataRegular)

    numDimensions = ws.getNumDims()
    numHistograms = ws.getNumberHistograms()
    print('This workspace has {0} dimensions and {1} histograms.'.format(numDimensions, numHistograms))

Output:

.. testoutput:: ExLoad

    This workspace has 2 dimensions and 98305 histograms.

**Example - Loading IN5 data scan**

.. testcode:: LoadILLTOF3IN5ScanExample

  ws = LoadILLTOF(Filename='ILL/IN5/199857.nxs')

  print('The output has {0} bin (omega scan) and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLTOF3IN5ScanExample

  The output has 17 bin (omega scan) and 98305 spectra

.. categories::

.. sourcelink::
