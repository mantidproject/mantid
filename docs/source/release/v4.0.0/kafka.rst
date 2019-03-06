====================
Kafka Live Listeners
====================

.. contents:: Table of Contents
   :local:
   
Event and Histogram Listeners
-----------------------------
There are two new mechanisms for listening to live data in Mantid using the Apache Kafka distributed streaming platform. The **KafkaEventListener** and the **KafkaHistoListener** allow users to 
listen on a kafka stream for event and histogram data respectively. The process for using the listeners can be found in the documentation for `StartLiveData <http://docs.mantidproject.org/nightly/algorithms/StartLiveData-v1.html>`_.

Example Usage
-------------
Below is an example of using the **KafkaEventListener** from a python script in Mantid:

.. code-block:: python

    StartLiveData(FromNow=False, FromStartOfRun=True, UpdateEvery=2, Instrument='SANS2D', 
                    Listener='KafkaEventListener', Address='sakura:9092', ProcessingAlgorithm='Rebin', 
                    ProcessingProperties='Params=0,1000,100000', AccumulationMethod='Replace',
                    RunTransitionBehavior='Restart', OutputWorkspace='testout')


:ref:`Release 4.0.0 <v4.0.0>`
