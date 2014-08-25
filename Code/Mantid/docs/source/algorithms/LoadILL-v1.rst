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

Usage
-----

**Example - Load a regular histogram Nexus file:**
(see :ref:`algm-LoadILL` for more options)

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
(see :ref:`algm-LoadILL` for more options)

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
(see :ref:`algm-LoadILL` for more options)

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
