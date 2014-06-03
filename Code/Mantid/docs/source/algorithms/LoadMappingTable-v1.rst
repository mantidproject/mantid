.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Loads the mapping table between spectra and `IDetector <IDetector>`__
from a RAW file. It fills the
`SpectraToDetectorMap <SpectraToDetectorMap>`__ object contained in a
`workspace <workspace>`__. This algorithm will fail if the
`workspace <workspace>`__ does not already point to a full
`instrument <instrument>`__ `geometry <geometry>`__ (which usually means
it must be run after
:ref:`algm-LoadInstrument`/:ref:`algm-LoadInstrumentFromRaw`).

The association is one to many, i.e. a spectrum can have one or many
detectors contributing to it. Alternatively the same spectrum can
contribute to different spectra (for example in DAE2 (Data Aquisition
Electronic) when a spectra containing electronically focussed data is
created simultaneously with individual spectra).

.. categories::
