===============
 ComponentInfo
===============

This is a python binding to the C++ class Mantid::Geometry::ComponentInfo.

--------
Purpose
--------
The purpose of the ComponentInfo object is to allow the user to access geometric information about the components which are part of beamline. A component is any physical item or group of items that is registered for the purpose of data reduction. The ComponentInfo object can be used to access information such as the total number of components in the beamline, the absolute position of a component as well as the absolute rotation of a component.

Many users may need this extra information so that they can have a better understanding of the beamline they are using and the components that make up the beamline - e.g. detectors. This extra information is also easy and fast to access meaning the users can make improvements to their experimental design with ease.

ComponentInfo is one of three objects that the user can gain access to from a Workspace. 
The other two are:
* SpectrumInfo
* DetectorInfo

-------
Usage
-------

**Example 1 - Creating a ComponentInfo Object:**
This example shows how to obtain a ComponentInfo object from a workspace object.
The return value is a ``ComponentInfo`` object.

.. testcode:: CreateComponentInfoObject
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the ComponentInfo object
	info = ws.componentInfo()
	print(type(info))

**Example 2 - Calling a method on the ComponentInfo Object:**
This example shows how to call the ``componentsInSubtree`` method.
The method takes in an integer ``index`` parameter which corresponds to a component.
The return value is a list of integers denoting components.

.. testcode:: CallRelativePositionMethod

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the ComponentInfo object
	info = ws.componentInfo()

	# Call relativePosition()
	print(type(info.relativePosition(0)))

**Example 3 - Calling a method on the ComponentInfo Object:**
The ``setRotation()`` method takes in a Quat object which defines a rotation. The rotation is applied to the component. Retriving the roatation after setting it may not always give the same Quat object back - i.e. the values could be changed.

.. testcode:: CallSetRotationMethod
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the ComponentInfo object
	info = ws.componentInfo()

	# Create a sample Quat and call setRotation
	quat = Quat(0, 0, 0, 0)
	info.setRotation(0, quat)
	print(info.rotation(0))

**Example 4 - Calling a method on the ComponentInfo Object:**
The ``hasParent()`` method takes an ``index`` parameter which represents the component and returns True if the component has a parent component.

.. testcode:: CallHasParent
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the ComponentInfo object
	info = ws.spectrumInfo()

	# Call the hasParent method
	print(info.hasParent(0))


**Example 5 - Retrieving a List of Child Components from a ComponentInfo Object:**
The ``children()`` method does not take in any parameters and returns a list of integers representing the child components. The returned list can then be indexed into to obtain a specific component.

.. testcode:: CallChildren
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the ComponentInfo object
	info = ws.componentInfo()

	# Get a list of the child components
	childComponents = info.children(0)
	print(type(childComponents))
	print(len(childComponents))

Output:

.. testoutput:: CreateComponentInfoObject

	<class 'mantid.api._api.ComponentInfo'>

.. testoutput:: CallRelativePositionMethod

	<class 'mantid.kernel._kernel.V3D'>

.. testoutput:: CallSetRotationMethod

	[1,0,0,0]
	[0,0,0,0]

.. testoutput:: CallHasParent

	True

.. testoutput:: CallChildren

	<type 'list'>
	0

	
*bases:* :py:obj:`mantid.geometry.ComponentInfo`

.. module:`mantid.geometry`

.. autoclass:: mantid.geometry.ComponentInfo 
    :members:
    :undoc-members:
    :inherited-members: