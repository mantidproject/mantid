.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm adds or replaces an parameter attached to an instrument
component, or the entire instrument. Instrument parameters are specific
to a workspace, they will get carried on to output workspaces created
from an input workspace to an algorithm, but will not appear on
unrelated workspaces that happen to have been recorded on the same
instrument.

The workspace must have a instrument already defined, and will be
altered in place. If the name of the instrument component to attach the
parameter is not specified it will be attached to the whole instrument.

At present this algorithm only supports simple instrument parameters,
NOT fitting parameters.

Parameter Types
---------------

The algorithm supports three types of parameters; `Number` (integer or floating point), `String` and `Bool`.
For `Bool` type, valid values are `1`, `0`, `true` or `false` (not case-sensitive).

Usage
-----

**Example - a few simple parameters**

.. testcode:: Ex1

  ws = CreateSampleWorkspace()
  #set a string parameter on the whole instrument
  SetInstrumentParameter(ws,ParameterName="TestParam",Value="Hello")

  #set a Number parameter just for bank 1
  SetInstrumentParameter(ws,ParameterName="NumberParam",Value="3", ComponentName="bank1",ParameterType="Number")

  #set a different value on bank 2
  SetInstrumentParameter(ws,ParameterName="NumberParam",Value="3.5", ComponentName="bank2",ParameterType="Number")

  compInfo=ws.componentInfo()
  bank1=compInfo.indexOfAny("bank1")
  bank2=compInfo.indexOfAny("bank2")

  print("The whole instrument parameter can be read from anywhere.")
  print("  The instrument: " + compInfo.getStringParameter("TestParam")[0])
  print("  bank 1: " + compInfo.getStringParameter("TestParam", bank1)[0])
  print("  bank 2: " + compInfo.getStringParameter("TestParam", bank2)[0])

  print("The parameters  on the Bank 1 can be read from the bank or below.")
  #For this one call getIntParameter as the number was an int
  print("  bank 1: " + str(compInfo.getIntParameter("NumberParam", bank1)[0]))
  #For this one call getNumberParameter as the number was a float
  print("  bank 2: " + str(compInfo.getNumberParameter("NumberParam", bank2)[0]))
  #if you are not sure of the type of a parameter you can call getParameterType
  print("  The type of NumberParam in bank 1: " + compInfo.getParameterType("NumberParam", bank1))
  print("  The type of NumberParam in bank 2: " + compInfo.getParameterType("NumberParam", bank2))


Output:

.. testoutput:: Ex1

    The whole instrument parameter can be read from anywhere.
      The instrument: Hello
      bank 1: Hello
      bank 2: Hello
    The parameters  on the Bank 1 can be read from the bank or below.
      bank 1: 3
      bank 2: 3.5
      The type of NumberParam in bank 1: int
      The type of NumberParam in bank 2: double

**Example - Overwriting existing values**

.. testcode:: Ex2

  ws = CreateSampleWorkspace()
  #set a string parameter on the whole instrument
  SetInstrumentParameter(ws,ParameterName="TestParam",Value="Hello")
  SetInstrumentParameter(ws,ParameterName="TestParam",Value="Goodbye")
  SetInstrumentParameter(ws,ParameterName="TestParam",Value="Hello from bank 1",ComponentName="bank1")

  compInfo=ws.componentInfo()
  bank1=compInfo.indexOfAny("bank1")

  print("The SetInstrumentParameter overwrites previous values where the ParameterName and Component match.")
  print("  The test param for the instrument is: " + compInfo.getStringParameter("TestParam")[0])
  print("Different Components can have the same Parameter Name with different values.")
  print("You will receive the closest value to the component you ask from.")
  print("  The test param for bank 1 is: " + compInfo.getStringParameter("TestParam", bank1)[0])

Output:

.. testoutput:: Ex2

    The SetInstrumentParameter overwrites previous values where the ParameterName and Component match.
      The test param for the instrument is: Goodbye
    Different Components can have the same Parameter Name with different values.
    You will receive the closest value to the component you ask from.
      The test param for bank 1 is: Hello from bank 1


.. categories::

.. sourcelink::
