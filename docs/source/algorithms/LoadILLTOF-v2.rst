.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL TOF NeXus file into a :ref:`Workspace2D <Workspace2D>` with
the given name.

To date this algorithm only supports: IN4, IN5 and IN6.

.. note::
    The initial time-of-flight axis is set up using the 'time_of_flight' field in the NeXus file. Therefore the conversion from 'TOF' to 'DeltaE' may not give the correct zero-energy transfer.

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

.. categories::

.. sourcelink::
