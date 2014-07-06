.. algorithm::

.. summary::

.. alias::

.. properties::

.. _MatrixWorkspace: http://www.mantidproject.org/MatrixWorkspace

Description
-----------

Saves the given `MatrixWorkspace`_ to a file in the NeXus-based 'NXSPE' format.

Restrictions on the Input Workspace
###################################

The input workspace must have units of Momentum Transfer ('DeltaE') and
contain histogram data with common binning on all spectra.

Child Algorithms Used
#####################

The :ref:`algm-FindDetectorsPar` algorithm is used to calculate
detectors parameters from the instrument description.

Usage
-----

**Example - Save/Load "Roundtrip"**

.. testcode:: ExSimpleSavingRoundtrip

   import os
   import numpy

   # Create dummy workspace.
   out_ws = CreateSimulationWorkspace(Instrument="IRIS", BinParams="0,500,2000")
   out_ws.setY(0, numpy.array([10.0, 50.0, 30.0, 60.0]))

   file_path = os.path.join(config["defaultsave.directory"], "NXSPEData.nxsps")

   # Do a "roundtrip" of the data.
   SaveNXSPE(out_ws, file_path)
   in_ws = LoadNXSPE(file_path)

   print "Contents of the first spectrum = " + str(in_ws.readY(0)) + "."

.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the first spectrum = [ 10.  50.  30.  60.].

.. categories::
