===============================
Listening on Kafka Live Streams
===============================

.. contents:: Table of Contents
   :local:
   
Kafka Listeners
---------------
The basic use of all listeners in Mantid is described in the documentation for :ref:`StartLiveData <algm-StartLiveData>`. Mantid provides two listener types for live streaming 
data using the Apache Kafka distributed streaming plaform. These listeners are used within the :ref:`StartLiveData <algm-StartLiveData>` algorithm internally. They cannot be invoked separately.

KafkaEventListener
##################

This listener is used for streaming live event data. Without using pre or post processing, this results in the creation of an :ref:`EventWorkspace <EventWorkspace>`. The listener address in `StartLiveData` should be set to the address of the kafka 
broker from which data will be streamed. The Kafka topic names for sample environment, run info etc. are all standardized and pre-configured and offset against the instrument name which is set by the 
:ref:`StartLiveData <algm-StartLiveData>` *Instrument* field. Users are not required to manually enter topic names. As with all event streams, if the `AccumulationMethod` is set to *Append* there is a danger of running out of memory.

This following python script shows how this listener is used:

.. code-block:: python

    # broker address can take the form hostname:port or ipaddress:port
    StartLiveData(FromNow=False, FromStartOfRun=True, UpdateEvery=2, Instrument='SANS2D', 
      Listener='KafkaEventListener', Address='kafkabroker:address', AccumulationMethod='Replace',
      RunTransitionBehavior='Restart', OutputWorkspace='testout')

KafkaHistoListener
##################

This listener is used for streaming histogram data. This will result in the creation of a :ref:`Workspace2D <Workspace2D>`. As with the `KafkaEventListener`, the listener address in `StartLiveData` should be set to the address of the kafka
broker. Please note that for this type of listener, events are accumulated for the instrument before streaming. Every new packet contains the historical accumulation of events so there is no neeed to manually keep track of
this by summing events. This will produce incorrect information. Topic names for sample environment, run info etc are all standardized, pre-configured and offset against the instrument name which is set by the 
:ref:`StartLiveData <algm-StartLiveData>` *Instrument* field.

This following python script shows how this listener is used:

.. code-block:: python

    # broker address can take the form hostname:port or ipaddress:port
    StartLiveData(FromNow=False, FromStartOfRun=True, UpdateEvery=1, Instrument='SANS2D', 
      Listener='KafkaHistoListener', Address='kafkabroker:address', AccumulationMethod='Replace',
      RunTransitionBehavior='Stop', OutputWorkspace='testout')


.. categories:: Concepts
