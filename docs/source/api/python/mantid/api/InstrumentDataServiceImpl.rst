.. _mantid.api.InstrumentDataServiceImpl:

===========================
 InstrumentDataServiceImpl
===========================

This is a Python binding to the C++ class Mantid::API::InstrumentDataServiceImpl.

What is it?
-----------

The Instrument Data Service is a :ref:`Data Service <Data Service>` that is
specialized to hold cached :py:obj:`Instruments <mantid.geometry.Instrument>`.
This cache is managed internally by Mantid when instruments are loaded through
algorithms.

Unlike the :py:obj:`AnalysisDataService <mantid.api.AnalysisDataServiceImpl>`,
the Python interface intentionally exposes only a limited set of methods.
Instruments should be added to the cache only by the framework, while Python is
restricted to querying, retrieving, and clearing cached entries.

Extracting an instrument from the Instrument Data Service
---------------------------------------------------------

The most usual way to populate the cache is to run an algorithm that loads an
instrument.

``LoadEmptyInstrument(InstrumentName="PG3")``

The list of cached instruments can be queried as follows:

``InstrumentDataService.getObjectNames()``

You can then access the cached instrument directly from the
InstrumentDataService.

``instrument = InstrumentDataService.retrieve("POWGEN**************")``

The returned object is a :py:obj:`mantid.geometry.Instrument`.


Reference
---------

.. module:`mantid.api`

.. autoclass:: mantid.api.InstrumentDataServiceImpl
    :members:
    :undoc-members:
    :inherited-members:
