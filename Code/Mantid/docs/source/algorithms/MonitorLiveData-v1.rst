.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The MonitorLiveData algorithm is started in the background by
:ref:`_algm-StartLiveData` and repeatedly calls
:ref:`_algm-LoadLiveData`. **It should not be necessary to call
MonitorLiveData directly.**

This algorithm simply calls :ref:`_algm-LoadLiveData` at the given
*UpdateFrequency*. For more details, see
:ref:`_algm-StartLiveData`.

For details on the way to specify the data processing steps, see:
`LoadLiveData <LoadLiveData#Description>`__.

.. categories::
