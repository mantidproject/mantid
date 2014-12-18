.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads an LLB MIBEMOL TOF NeXus file into a :ref:`Workspace2D <Workspace2D>`
with the given name.

This loader calculates the elastic peak position (EPP) on the fly.

To date this algorithm only supports the MIBEMOL instrument.

Usage
-----

**Example - Load a LLB MIBEMOL NeXus file:**
(see :ref:`algm-LoadLLB` for more options)

.. code-block:: python

   # Load the MIBEMOL dataset into a workspace2D.
   ws = Load('LLB_d22418.nxs')

   print "This workspace has", ws.getNumDims(), "dimensions and its title is:", ws.getTitle()

Output:

   This workspace has 2 dimensions and its title is: Lysozyme/D2O (c=80 mg/ml) Treg=293 K, Tch=288 K  wl=5.2A  sous pression (4.5 kbar)

.. categories::
