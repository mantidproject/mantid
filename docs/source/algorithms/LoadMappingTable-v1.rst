.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads the mapping table between spectra and detectors from a RAW file. It fills
the spectra to detector map object contained in a :ref:`workspace <workspace>`.
This algorithm will fail if the :ref:`workspace <workspace>` does not already
point to a full :ref:`instrument <instrument>` :ref:`geometry <geometry>` (which
usually means it must be run after :ref:`algm-LoadInstrument` or
:ref:`algm-LoadInstrumentFromRaw`).

The association is one to many, i.e. a spectrum can have one or many
detectors contributing to it. Alternatively the same spectrum can
contribute to different spectra (for example in DAE2 (Data Aquisition
Electronic) when a spectra containing electronically focussed data is
created simultaneously with individual spectra).

Usage
-----

.. testcode::
  
  # Create a workspace
  ws = CreateSampleWorkspace()
  # Replace the instrument in the workspace with HRPD
  LoadInstrument(ws,InstrumentName='HRPD', RewriteSpectraMap=True)
  # Map spectra to detectors according to an HRPD raw file.
  LoadMappingTable('HRP39180.RAW',ws)

.. categories::

.. sourcelink::
