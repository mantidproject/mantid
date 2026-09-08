.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads the mapping table between spectra and detectors from a RAW file. It fills
the spectra to detector map object contained in a :py:obj:`~mantid.api.Workspace`.
This algorithm will fail if the :py:obj:`~mantid.api.Workspace` does not already
point to a full :py:obj:`Instrument <mantid.geometry.Instrument>` :ref:`geometry <geometry>` (which
usually means it must be run after :ref:`algm-LoadInstrument` or
:ref:`algm-LoadInstrumentFromRaw`).

The association is one to many, i.e. a spectrum can have one or many
detectors contributing to it. Alternatively the same spectrum can
contribute to different spectra (for example in DAE2 (Data Acquisition
Electronic) when a spectra containing electronically focussed data is
created simultaneously with individual spectra).

Where the instrument definition sets ``spectra-map-source`` to ``instrument`` in its
:ref:`parameter file <InstrumentParameterFile>`, the table read from the file is corrected against the instrument
before it is applied. See :ref:`the LoadRaw description <raw-spectra-map-source>` for what that does and when it
applies.

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
