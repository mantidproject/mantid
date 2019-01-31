.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


GetLiveInstrumentValue
----------------------

This algorithm gets live values such as run title or theta from an instrument.

CaChannel must be installed for this algorithm to work. See the instructions `here <https://www.mantidproject.org/CaChannel_In_Mantid>`_.

The instrument must also be on IBEX or have additional processes installed to supply the EPICS values. If it does not, you will get an error that the requested value could not be found.


Usage
-------

    GetLiveInstrumentValue(Instrument='INTER',PropertyType='Block',PropertyName='Theta')

.. seealso :: Algorithm :ref:`algm-ReflectometryReductionOneLiveData`

.. categories::

.. sourcelink::
