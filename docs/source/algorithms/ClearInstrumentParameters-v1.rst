.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm clears all the parameters associated with a workspace's instrument.

Parameters are used by Mantid to tweak an instrument's values without having to change
the :ref:`instrument definition file <InstrumentDefinitionFile>` itself.

Usage
-----

.. testcode::

  ws = CreateSampleWorkspace()

  #Set a string parameter on the whole instrument
  SetInstrumentParameter(ws,ParameterName="TestParam",Value="Hello")

  #Set a Number parameter just for bank 1
  SetInstrumentParameter(ws,ParameterName="NumberParam",Value="3", ComponentName="bank1",ParameterType="Number")

  #Set a different value on bank 2
  SetInstrumentParameter(ws,ParameterName="NumberParam",Value="3.5", ComponentName="bank2",ParameterType="Number")

  compInfo = ws.componentInfo()
  bank1 = compInfo.indexOfAny("bank1")
  bank2 = compInfo.indexOfAny("bank2")

  #Check the parameters are set correctly
  print("Instrument: " + compInfo.getStringParameter("TestParam")[0])
  print("Bank1: " + compInfo.getStringParameter("TestParam", bank1)[0])
  print("Bank2: " + compInfo.getStringParameter("TestParam", bank2)[0])

  #Clear all the instrument's parameters
  print("Clearing all parameters")
  ClearInstrumentParameters(ws)

  #Check the parameters have been cleared correctly
  #Obtain the component info again, to make sure it contains the updated parameters
  compInfo = ws.componentInfo()
  if len(compInfo.getStringParameter("TestParam")) == 0:
    print("Instrument was cleared successfully.")
  if len(compInfo.getStringParameter("TestParam", bank1)) == 0:
    print("Bank1 was cleared successfully.")
  if len(compInfo.getStringParameter("TestParam", bank2)) == 0:
    print("Bank2 was cleared successfully.")

.. testoutput::

  Instrument: Hello
  Bank1: Hello
  Bank2: Hello
  Clearing all parameters
  Instrument was cleared successfully.
  Bank1 was cleared successfully.
  Bank2 was cleared successfully.

.. categories::

.. sourcelink::
