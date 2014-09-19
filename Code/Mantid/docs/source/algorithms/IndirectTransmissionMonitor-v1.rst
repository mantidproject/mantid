.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates the sample transmission using the raw data files of the sample and its background or container.

For more details, see `Indirect:Transmission <http://www.mantidproject.org/Indirect:Transmission>`_.

Output workspace will default to [instument][run number]_[analyser][reflection]_Transmission if not set.

Usage
-----

**Example - Create mapping file for IRIS**

.. testcode:: exIRISTransmission

   sample_file = 'IRS26176.RAW'
   can_file = 'IRS26173.RAW'

   sample_ws = Load(sample_file)
   can_ws = Load(can_file)

   transmission_ws = IndirectTransmissionMonitor(SampleWorkspace=sample_ws, CanWorkspace=can_ws)

   print transmission_ws.getNames()

**Output:**

.. testoutput:: exIRISTransmission

   ['sample_ws_Sam','sample_ws_Can','sample_ws_Trans']

.. categories::
