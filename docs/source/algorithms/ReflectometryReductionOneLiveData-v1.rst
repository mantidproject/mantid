.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


ReflectometryReductionOneAutoLiveData
-------------------------------------

This algorithm performs a reduction with :ref:`algm-ReflectometryReductionOneAuto` on a live data workspace. It is intended to be run as a post-processing algorithm for :ref:`algm-StartLiveData`, although it can also be called directly on a workspace output from live data monitoring. It is not intended to be run on a workspace for a completed run.

This algorithm does some setting up of the instrument and sample logs, which are not normally present for a live data workspace, so that the reduction can be run. This uses live values for ``theta`` and the slit gaps, which are found from the instrument using :ref:`algm-GetLiveInstrumentValue`. Once the workspace is set up, :ref:`algm-ReflectometryReductionOneAuto` is run, with ``ThetaLogName`` set to the appropriate value to use the value of ``theta`` that was set in the logs.

:ref:`algm-GetLiveInstrumentValue` requires Mantid to have EPICS support installed, and appropriate processes must be running on the instrument to supply the EPICS values. A different algorithm for fetching live values could be specified by overriding the ``GetLiveValueAlgorithm`` property.

Usage
-------

    StartLiveData(Instrument='INTER',
        PostProcessingAlgorithm='ReflectometryReductionOneLiveData', PostProcessingProperties='Instrument=INTER',
        AccumulationMethod='Replace',AccumulationWorkspace='TOF_live',OutputWorkspace='IvsQ_binned_live',)

.. seealso :: Algorithm :ref:`algm-GetLiveInstrumentValue`, :ref:`algm-ReflectometryReductionOneAuto`, :ref:`algm-StartLiveData` and the ``ISIS Reflectometry`` interface.

.. categories::

.. sourcelink::
