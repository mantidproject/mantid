==============
 SpectrumInfo
==============

This a python binding to the C++ class Mantid::API::SpectrumInfo.

--------  
Purpose 
-------- 
The purpose of the SpectrumInfo object is to allow the user to access information about the spectra being used in an experiment. The SpectrumInfo object can be used to access information such as the number of spectra, the absolute position of a spectrum as well as the distance from the sample to the source. There are many other methods available as well. 
 
Many users may need this extra information so that they can have a better understanding of the instrument they are using. This extra information is also easy and fast to access meaning the users can make improvements to their experimental design with ease. 

SpectrumInfo is one of three objects that the user can gain access to from a Workspace. 
The other two are:  
* DetectorInfo 
* ComponentInfo

------
Usage
------

**Example 1 - Creating a SpectrumInfo Object:**
This example shows how to obtain a SpectrumInfo object from a workspace object.
The return value is a ``SpectrumInfo`` object.

.. testcode:: CreateSpectrumInfoObject
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()
	print(type(info))

**Example 2 - Calling a method on the SpectrumInfo Object:**
This example shows how to call the ``hasDetectors`` method.
The method takes in an integer ``index`` parameter which corresponds to a spectrum.
The return value is True or False.

.. testcode:: CallHasDetectorsMethod

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Call hasDetectors()
	print(info.hasDetectors())

**Example 3 - Calling a method on the SpectrumInfo Object:**
The ``l1()`` method does not take in any parameters and returns the distance from the source to the sample.

.. testcode:: GetL1Value
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Get the l1 value
	print(info.l1())

**Example 4 - Calling a method on the SpectrumInfo Object:**
The ``sourcePosition()`` method does not take any parameters and returns the absolute source position. The return value is a ``V3D`` object which gives a position in 3D space.

.. testcode:: GetSourcePosition
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Get the source position
	print(type(info.sourcePosition()))

**Example 5 - Retrieving SpectrumDefinition Objects from a SpectrumInfo Object:**
The ``getAllSpectrumDefinitions()`` method does not take in any parameters and returns a list of ``SpectrumDefinition``s. The returned list can then be indexed into to obtain specific ``SpectrumDefinition`` objects.

.. testcode:: GetAllSpectrumDefintions
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Get the SpectrumDefinition objects
	allSpectrumDefinitions = info.getAllSpectrumDefinitions()
	print(type(allSpectrumDefinitions))
	print(type(allSpectrumDefinitions[0]))

Output:

.. testoutput:: CreateSpectrumInfoObject

	<class 'mantid.api._api.SpectrumInfo'>

.. testoutput:: CallHasDetectorsMethod

	False

.. testoutput:: GetL1Value

	10.0

.. testoutput:: GetSourcePosition

	<class 'mantid.kernel._kernel.V3D'>

.. testoutput:: GetAllSpectrumDefinitions

	<type 'list'>
	<class 'mantid.api._api.SpectrumDefinition'>



*bases:* :py:obj:`mantid.api.SpectrumInfo`

.. module:`mantid.api`

.. autoclass:: mantid.api.SpectrumInfo 
    :members:
    :undoc-members:
    :inherited-members:

