.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


GetLiveInstrumentValue
----------------------

This algorithm gets live values such as run title or theta from an instrument.

Values are accessed via EPICS, so EPICS support must be installed in Mantid for this algorithm to work. This is included by default on Windows but see the instructions `here <https://www.mantidproject.org/PyEpics_In_Mantid>`_ for other platforms.

The instrument must also be on IBEX or have additional processes installed to supply the EPICS values. If it does not, you will get an error that the requested value could not be found.


Usage
-------

    GetLiveInstrumentValue(Instrument='INTER',PropertyType='Block',PropertyName='Theta')

.. seealso :: Algorithm :ref:`algm-ReflectometryReductionOneLiveData`

.. categories::

.. sourcelink::
