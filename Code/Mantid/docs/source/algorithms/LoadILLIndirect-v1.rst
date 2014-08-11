.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an ILL Back Scattering NeXus file into a :ref:`Workspace2D <Workspace2D>` with
the given name.

The Units axis is defined with *empty* units. The main purpose of this loader is to be used with the **Indirect Load** interface.

To date this algorithm only supports: IN16B

Usage
-----

**Example - Load ILL IN16B NeXus file:**

.. code-block:: python

   # Load ILL IN16B data file into a workspace 2D.
   ws = Load('ILLIN16B_034745.nxs')

   print "This workspace has", ws.getNumDims(), "dimensions and has", ws.getNumberHistograms(), "histograms."

Output:

	This workspace has 2 dimensions and has 2057 histograms.

.. categories::
