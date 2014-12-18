.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm clears all the parameters associated with a workspace's instrument.

Parameters are used by Mantid to tweak an instrument's values without having to change
the `instrument definition file <http://mantidproject.org/InstrumentDefinitionFile>`__ itself.

The LocationParameters property specifies whether or not to clear any calibration parameters
used to adjust the location of any components. Specifically, it will clear the "x", "y", "z",
"r-position", "t-position", "p-position", "rotx", "roty", and "rotz" parameters.

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

  instrument = ws.getInstrument()
  bank1 = instrument.getComponentByName("bank1")
  bank2 = instrument.getComponentByName("bank2")

  #Check the parameters are set correctly
  print("Instrument: " + instrument.getStringParameter("TestParam")[0])
  print("Bank1: " + bank1.getStringParameter("TestParam")[0])
  print("Bank2: " + bank2.getStringParameter("TestParam")[0])

  #Clear all the instrument's parameters
  print("Clearing all parameters")
  ClearInstrumentParameters(ws)

  #Check the parmaeters have been cleared correctly
  if len(instrument.getStringParameter("TestParam")) == 0:
    print("Instrument was cleared successfully.")
  if len(bank1.getStringParameter("TestParam")) == 0:
    print("Bank1 was cleared successfully.")
  if len(bank2.getStringParameter("TestParam")) == 0:
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
