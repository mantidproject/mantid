.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL TOF NeXus file into a :ref:`Workspace2D <Workspace2D>` with
the given name.

This loader calculates the elastic peak position (EPP) on the fly. In
cases where the dispersion peak might be higher than the EPP, it is good
practice to load a Vanadium file.

The property FilenameVanadium is optional. If it is present the EPP will
be loaded from the Vanadium data.

To date this algorithm only supports: IN4, IN5 and IN6

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

**Example - Load a regular histogram Nexus file:**
(see :ref:`algm-LoadILLTOF` for more options)

.. code-block:: python

   # Regular data file.
   dataRegular = 'ILLIN6_151460.nxs'

   # Load ILL dataset
   ws = Load(dataRegular)

   print "This workspace has", ws.getNumDims(), "dimensions and has", ws.getNumberHistograms(), "histograms."

Output:

   This workspace has 2 dimensions and has 340 histograms.



**Example - Load a histogram Nexus file where the dispersion peak is higher than the elastic peak.
An auxiliary vanadium file is needed to locate the elastic peak.:**
(see :ref:`algm-LoadILLTOF` for more options)

.. code-block:: python

   # Data file where the dispersion peak is higher than the elastic peak.
   dataDispersionPeak = 'ILLIN5_Sample_096003.nxs'

   # Vanadium file collected in the same conditions as the dispersion peak dataset.
   vanaDispersionPeak = 'ILLIN5_Vana_095893.nxs'

   # Load ILL dispersion peak dataset and a vanadium dataset
   ws = Load(dataDispersionPeak, FilenameVanadium=vanaDispersionPeak)

   print "This workspace has", ws.getNumDims(), "dimensions and has", ws.getNumberHistograms(), "histograms."

Output:

   This workspace has 2 dimensions and has 98305 histograms.

**Example - Same example as above, but the vanadium file is loaded in advance. The dataset for the dispersion peak is loaded after, using the auxiliary vanadium workspace.:**
(see :ref:`algm-LoadILLTOF` for more options)

.. code-block:: python

   # Data file where the dispersion peak is higher than the elastic peak.
   dataDispersionPeak = 'ILLIN5_Sample_096003.nxs'

   # Vanadium file collected in the same conditions as the dispersion peak dataset.
   vanaDispersionPeak = 'ILLIN5_Vana_095893.nxs'

   # Load the Vanadium
   wsVana = Load(dataDispersionPeak)

   # Load ILL dispersion peak dataset and a vanadium dataset
   wsData = Load(dataDispersionPeak, WorkspaceVanadium=wsVana)

   print "The Vanadium workspace has", wsVana.getNumDims(), "dimensions and has", wsVana.getNumberHistograms(), "histograms."
   print "The Data workspace has", wsData.getNumDims(), "dimensions and has", wsData.getNumberHistograms(), "histograms."

Output:

	The Vanadium workspace has 2 dimensions and has 98305 histograms.
	The Data workspace has 2 dimensions and has 98305 histograms.

.. categories::

.. sourcelink::
