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

Topic Configuration
###################

The topics that the listener will subscribe to are defined for a particular instrument in Mantid's "Facilities.xml" file. Here is the entry for the V20 beamline for example:

.. code-block:: xml

    <instrument name="V20" shortname="V20" >
      <technique>ESS Test Beamline</technique>
      <livedata default="event">
        <connection name="event" address="192.168.1.80:9092" listener="KafkaEventListener" />
     <topic name="V20_choppers" type="chopper" />
     <topic name="V20_motion" type="sample" />
     <topic name="V20_runInfo" type="run" />
     <topic name="denex_detector" type="event" />
     <topic name="monitor" type="monitor" />
      </livedata>
    </instrument>

Data in the Kafka topics is serialised using Google FlatBuffers, according to schema which can be found in the `ESS Streaming-Data-Types repository <https://github.com/ess-dmsc/streaming-data-types>`.
The FlatBuffer Compiler tool generates C++ code for each schema file to provide an implementation for serialising and deserialising data to the format it is communicated in, over the network, through Kafka.
These generated C++ files are included in the Mantid source.
Each schema is identified by a four character string, for example "ev42" which identifies the schema defining the serialised data format for detection event data.
The schema identifiers are defined in their corresponding schema file, and are included in the schema file name and the generated C++ code filenames.
Particular serialised data are expected to be found in different topics on Kafka. The schema identifier, or identifiers, for data in each topic "type" are documented in the table below.

.. list-table:: Topic configuration
   :widths: 15 15 30 30
   :header-rows: 1

   * - Type
     - Schema (see https://github.com/ess-dmsc/streaming-data-types)
     - Required
     - Description
   * - chopper
     - tdct
     - No (topic doesn't have to exist)
     - Neutron chopper top-dead-centre timestamps
   * - sample
     - f142
     - Yes (but topic can be empty)
     - Used to populate workspace logs. "sample" from "sample environment" which is the typical source of these data.
   * - run
     - pl72, 6s4t
     - Yes (there must be a pl72 run start message on the topic for the listener to start successfully)
     - Row 1, column 3
   * - event
     - ev42
     - Yes (but topic can be empty)
     - Detection event data
   * - monitor
     - ev42
     - Yes (but topic can be empty)
     - Detection event data from monitors (single pixel detectors). This just allows using a separate topic for these data, alternatively they can be published to the "event" topic with other data from other detectors.

Note, there must be a run start message (schema pl72) available in the "run" topic for the listener to start.
If the "nexus_structure" field of this message contains geometry information in NeXus format (NXoff_geometry or NXcylindrical_geometry) then Mantid will parse this to get the instrument geometry and expected detector ids etc.
Otherwise it uses the "instrument_name" to look up a Mantid Instrument Definition File (IDF) for the instrument. This behaviour is consistent with the :ref:`LoadInstrument <algm-LoadInstrument>` algorithm.
Comments in the pl72 schema file may be useful, in particular it documents which fields need to be populated to use the Mantid streamer and which are required by other software:
https://github.com/ess-dmsc/streaming-data-types/blob/master/schemas/pl72_run_start.fbs


.. categories:: Concepts
