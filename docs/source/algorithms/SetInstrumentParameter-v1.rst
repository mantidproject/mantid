.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm adds or replaces an parameter attached to an instrument
component, or the entire instrument. Instrument parameters are specific
to a workspace, they will get carried on to output workspaces created
from an input workspace to an algorithm, but will not appear one
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

  instrument=ws.getInstrument()
  bank1=instrument.getComponentByName("bank1")
  bank2=instrument.getComponentByName("bank2")

  print ("The whole instrument parameter can be read from anywhere.")
  print ("  The instrument: " + instrument.getStringParameter("TestParam")[0])
  print ("  bank 1: " + bank1.getStringParameter("TestParam")[0])
  print ("  bank 2: " + bank2.getStringParameter("TestParam")[0])

  print ("The parameters  on the Bank 1 can be read from the bank or below.")
  #For this one call getIntParameter as the number was an int
  print ("  bank 1: " + str(bank1.getIntParameter("NumberParam")[0]))
  #For this one call getNumberParameter as the number was a float
  print ("  bank 2: " + str(bank2.getNumberParameter("NumberParam")[0]))
  #if you are not sure of the type of a parameter you can call getParameterType
  print ("  The type of NumberParam in bank 1: " + bank1.getParameterType("NumberParam"))  
  print ("  The type of NumberParam in bank 2: " + bank2.getParameterType("NumberParam"))


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

  instrument=ws.getInstrument()
  bank1=instrument.getComponentByName("bank1")
  
  print ("The SetInstrumentParameter overwrites previous values where the ParameterName and Component match.")
  print ("  The test param for the instrument is: " + instrument.getStringParameter("TestParam")[0])
  print ("Different Components can have the same Parameter Name with different values.") 
  print ("You will receive the closest value to the component you ask from.")
  print ("  The test param for bank 1 is: " + bank1.getStringParameter("TestParam")[0])

Output:

.. testoutput:: Ex2

    The SetInstrumentParameter overwrites previous values where the ParameterName and Component match.
      The test param for the instrument is: Goodbye
    Different Components can have the same Parameter Name with different values.
    You will receive the closest value to the component you ask from.
      The test param for bank 1 is: Hello from bank 1


.. categories::

.. sourcelink::
