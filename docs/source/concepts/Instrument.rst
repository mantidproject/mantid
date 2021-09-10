.. _Instrument:

Instrument
==========

.. contents::
  :local:

What are Instruments?
---------------------

The Instrument in Mantid is a geometrical description of the components that make
up the beam line. The components described will generally include:

-  The source
-  The sample position
-  Each detector 'pixel'
-  Each monitor

Other components may also be included such as

-  Slits
-  Mirrows
-  Guides
-  Choppers
-  Engineering obstacles in the beam path
-  Link between log-files and variable parameters of the instrument
   (such as the height of a detector table)

An instrument is described using an :ref:`instrument definition
file <InstrumentDefinitionFile>`.

The Mantid geometry is further explained :ref:`here <Geometry>`.

**Why do we have a full instrument description, and not just a list of L2 and 2Theta values?**

A list of L2 and 2Theta values will provide information to perform unit
conversions and several other algorithms, however a full geometric
instrument description allows much more.

-  Visualization of the instrument internals with data overlays
-  Complex absorption corrections
-  Montecarlo simulations of experiments
-  Updating the instrument geometry according to values stored in
   log-files

Working with Instruments in Python
----------------------------------

Getting the Instrument from a workspace
#######################################

You can get access to the Instrument for a workspace with

.. testsetup:: WorkspaceInstrument

  ws = CreateSampleWorkspace()

.. testcode:: WorkspaceInstrument

    instrument = ws.getInstrument()

.. testoutput:: WorkspaceInstrument
  :hide:


Instrument Properties
#####################

.. testcode:: InstrumentPropertiestest

    ws = CreateSampleWorkspace()
    instrument = ws.getInstrument()

    # get the instrument name
    print(instrument.getName())
    # Get the validity dates for this instrument definition
    print(instrument.getValidToDate())
    print(instrument.getValidFromDate())
    # Get the X,Y,Z position of the source and sample
    source = instrument.getSource()
    sample = instrument.getSample()
    print(source.getPos())
    print(sample.getPos())
    # Get the distance from the source to the sample
    print(sample.getDistance(source))

.. testoutput:: InstrumentPropertiestest
    :hide:
    :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

    basic_rect
    ...
    10.0

Finding Specific Components of the instrument
#############################################

The instrument class has several methods to help in finding the objects that describe specific parts of the instrument.

.. testcode:: InstrumentComponents

    ws = CreateSampleWorkspace()
    instrument = ws.getInstrument()

    # Get the source and sample
    source = instrument.getSource()
    sample = instrument.getSample()

    # You can get a component by name
    bank1 = instrument.getComponentByName("bank1")
    # Or by Detector_id
    det101 = instrument.getDetector(101)

.. testoutput:: InstrumentProperties
  :hide:

Instrument Parameters
#####################

Instruments, or any component within them (bank, detector, chopper, slit etc) can have parameters defined for them.  These can be accessed from Python.  Any search for instrument parameters cascades up the instrument tree, so a detector will inherit any parameters from it's back, and it's instrument.

.. testcode:: InstrumentParameters

    # setup
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

    print("The whole instrument parameter can be read from anywhere.")
    print("  The instrument: " + instrument.getStringParameter("TestParam")[0])
    print("  bank 1: " + bank1.getStringParameter("TestParam")[0])
    print("  bank 2: " + bank2.getStringParameter("TestParam")[0])

    print("The parameters  on the Bank 1 can be read from the bank or below.")
    #For this one call getIntParameter as the number was an int
    print("  bank 1: " + str(bank1.getIntParameter("NumberParam")[0]))
    #For this one call getNumberParameter as the number was a float
    print("  bank 2: " + str(bank2.getNumberParameter("NumberParam")[0]))
    #if you are not sure of the type of a parameter you can call getParameterType
    print("  The type of NumberParam in bank 1: " + bank1.getParameterType("NumberParam"))
    print("  The type of NumberParam in bank 2: " + bank2.getParameterType("NumberParam"))

Output:

.. testoutput:: InstrumentParameters

    The whole instrument parameter can be read from anywhere.
      The instrument: Hello
      bank 1: Hello
      bank 2: Hello
    The parameters  on the Bank 1 can be read from the bank or below.
      bank 1: 3
      bank 2: 3.5
      The type of NumberParam in bank 1: int
      The type of NumberParam in bank 2: double



Getting all the Parameters on an instrument component
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. testsetup:: InstrumentParametersList

    # setup
    ws = CreateSampleWorkspace()
    #set a string parameter on the whole instrument
    SetInstrumentParameter(ws,ParameterName="TestParam",Value="Hello")

    #set a Number parameter just for bank 1
    SetInstrumentParameter(ws,ParameterName="NumberParam",Value="3", ComponentName="bank1",ParameterType="Number")

    #set a different value on bank 2
    SetInstrumentParameter(ws,ParameterName="NumberParam",Value="3.5", ComponentName="bank2",ParameterType="Number")

.. testcode:: InstrumentParametersList

    # setup as above
    instrument=ws.getInstrument()
    det101=instrument.getDetector(101)

    for name in det101.getParameterNames() :
        if det101.getParameterType(name) == "int":
            value = det101.getIntParameter(name)
        if det101.getParameterType(name) == "double":
            value = det101.getNumberParameter(name)
        if det101.getParameterType(name) == "string":
            value = det101.getStringParameter(name)
        print("{0} {1}".format(name,value))


Output:

.. testoutput:: InstrumentParametersList

    NumberParam [3]
    TestParam ['Hello']

.. categories:: Concepts
