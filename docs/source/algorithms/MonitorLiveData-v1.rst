.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The MonitorLiveData algorithm is started in the background by
:ref:`algm-StartLiveData` and repeatedly calls
:ref:`algm-LoadLiveData`. **It should not be necessary to call
MonitorLiveData directly.**

This algorithm simply calls :ref:`algm-LoadLiveData` at the given
*UpdateFrequency*. For more details, see
:ref:`algm-StartLiveData`.

For details on the way to specify the data processing steps, see:
`LoadLiveData <LoadLiveData#Description>`__.

Usage
-----

LoadLiveData is not intended for usage directly, it is part of he process that is started using :ref:`algm-StartLiveData`.

.. categories::

.. sourcelink::
