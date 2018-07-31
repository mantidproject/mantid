==============
 SpectrumInfo
==============

This a python binding to the C++ class Mantid::API::SpectrumInfo.

Most of the information concerning ``SpectrumInfo`` can be found in the `Instrument Access Layers <https://github.com/mantidproject/mantid/blob/9e3d799d40fda4a5ca08887e8c47f41c3316da91/docs/source/concepts/InstrumentAccessLayers.rst>`_ document. 

--------  
Purpose 
-------- 
The purpose of the ``SpectrumInfo`` object is to allow the user to access information about the spectra being used in an experiment. The ``SpectrumInfo`` object can be used to access information such as the number of spectra, the absolute position of a spectrum as well as the distance from the sample to the source. There are many other methods available as well. 

A spectrum corresponds to (a group of) one or more detectors. However if no instrument/beamline has been set then the number of detectors will be zero. An example test case details this below.  
 
Many users may need more information about the spectra in an experiment so that they can have a better understanding of the beamline they are using. This information is easy and fast to access via ``SpectrumInfo``. 

``SpectrumInfo`` is one of three objects that the user can gain access to from a workspace object.
The other two are:

* ``DetectorInfo``
* ``ComponentInfo``

------
Usage
------

**Example 1 - Creating a SpectrumInfo Object:**
This example shows how to obtain a ``SpectrumInfo`` object from a workspace object.
The return value is a ``SpectrumInfo`` object.

.. testcode:: CreateSpectrumInfoObject
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()
	print(type(info))

Output:

.. testoutput:: CreateSpectrumInfoObject

	<class 'mantid.api._api.SpectrumInfo'>


**Example 2 - Calling the hasDetectors Method on the SpectrumInfo Object:**
This example shows how to call the ``hasDetectors`` method.
The method takes in an integer ``index`` parameter which corresponds to a spectrum.
The return value is True or False.

.. testcode:: CallHasDetectorsMethod

	# Create a workspace to use (should have a preset instrument)
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Call hasDetectors
	print(info.hasDetectors(0))	

	"""
	The object returned by CreateWorkspace does not have an instrument set so 
	it is expected that we would be getting False back from hasDetectors(). 
	"""

	# Sample data
	intx = [1,2,3,4,5]
	inty = [1,2,3,4,5]

	# Create a workspace to use
	wsTwo = CreateWorkspace(intx, inty)

	# Get the SpectrumInfo object
	info = wsTwo.spectrumInfo()

	# Call hasDetectors
	print(info.hasDetectors(0))

Output:

.. testoutput:: CallHasDetectorsMethod

	True
	False


**Example 3 - Calling the l1 Method on the SpectrumInfo Object:**
The ``l1()`` method does not take in any parameters and returns the distance from the source to the sample.
The return value is a float.

.. testcode:: CallL1Method
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Call l1
	print(info.l1())

Output:

.. testoutput:: CallL1Method

	10.0


**Example 4 - Calling the sourcePosition method on the SpectrumInfo Object:**
The ``sourcePosition()`` method does not take any parameters and returns the absolute source position. 
The return value is a ``V3D`` object which is a position in 3D space.

.. testcode:: CallSourcePositionMethod
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Call sourcePosition
	print(info.sourcePosition())

Output:

.. testoutput:: CallSourcePositionMethod

	[0,0,-10]


**Example 5 - Retrieving SpectrumDefinition Objects from a SpectrumInfo Object:**
The ``getAllSpectrumDefinitions()`` method does not take in any parameters and returns a list of ``SpectrumDefinition``s. 
The returned list can then be indexed into to obtain specific ``SpectrumDefinition`` objects.

.. testcode:: CallGetAllSpectrumDefinitionsMethod
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Get the SpectrumDefinition objects
	allSpectrumDefinitions = info.getAllSpectrumDefinitions()
	print(len(allSpectrumDefinitions))
	print(type(allSpectrumDefinitions[0]))

Output:

.. testoutput:: CallGetAllSpectrumDefinitionsMethod
	
	200
	<class 'mantid.api._api.SpectrumDefinition'>


*bases:* :py:obj:`mantid.api.SpectrumInfo`

.. module:`mantid.api`

.. autoclass:: mantid.api.SpectrumInfo 
    :members:
    :undoc-members:
    :inherited-members:

