===============
 ComponentInfo
===============

This is a python binding to the C++ class Mantid::Geometry::ComponentInfo.

Most of the information concerning :py:obj:`~mantid.geometry.ComponentInfo` can be found in the :ref:`Instrument Access Layers <InstrumentAccessLayers>` document.

--------
Purpose
--------
The purpose of the :py:obj:`~mantid.geometry.ComponentInfo` object is to allow the user to access geometric information about the components which are part of a beamline. A component is any physical item or group of items that is registered for the purpose of data reduction. The :py:obj:`~mantid.geometry.ComponentInfo` object can be used to access information such as the total number of components in the beamline, the absolute position of a component as well as the absolute rotation of a component. :py:obj:`~mantid.geometry.ComponentInfo` provides tree like access to the beamline including all the detectors.

Many users may need this extra information so that they can have a better understanding of the beamline they are using and the components that make up the beamline - e.g. detectors, banks, choppers. This extra information is easy and fast to access.

:py:obj:`~mantid.geometry.ComponentInfo` is one of three objects that the user can gain access to from a workspace.
The other two are:

 * :py:obj:`~mantid.api.SpectrumInfo`
 * :py:obj:`~mantid.geometry.DetectorInfo`

---------
Indexing
---------
The :py:obj:`~mantid.geometry.ComponentInfo` object is accessed by an index going from 0 to N-1 where N is the number of components.
The component index for a detector is EQUAL to the detector index. In other words, a detector with a detector index of 5 when working with a ``DetectorInfo`` object and will have a component index of 5 when working with a :py:obj:`~mantid.geometry.ComponentInfo` object.

Another way to think about this is that the first 0 to n-1 components referenced in :py:obj:`~mantid.geometry.ComponentInfo` are detectors, where n is the total number of detectors.

-------
Usage
-------

**Example 1 - Creating a ComponentInfo Object:**
This example shows how to obtain a :py:obj:`~mantid.geometry.ComponentInfo` object from a workspace object.
The return value is a :py:obj:`~mantid.geometry.ComponentInfo` object.

.. testcode:: CreateComponentInfoObject

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the ComponentInfo object
	info = ws.componentInfo()
	from mantid.geometry import ComponentInfo
	print("The type is ComponentInfo: {}".format(isinstance(info, ComponentInfo)))

Output:

.. testoutput:: CreateComponentInfoObject

	The type is ComponentInfo: True


**Example 2 - Calling Some Methods on the ComponentInfo Object:**
This example shows how to call a few different methods on the ComponentInfo object.

The ``relativePosition`` method takes in an integer ``index`` parameter which corresponds to a component.
The return value is a ``V3D`` object which denotes a point in 3D space.

The ``setRotation()`` method takes in a ``Quat`` object which defines a rotation. The rotation is applied to the component.
Retrieving the rotation after setting it may not always give the same ``Quat`` object back - i.e. the values could be changed.

The ``hasParent()`` method takes an integer ``index`` parameter which corresponds to a component.
The return value is ``True`` if the component has a parent component or ``False`` otherwise.

.. testcode:: CallMethods

	# Import Quat
	from mantid.kernel import Quat

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the ComponentInfo object
	info = ws.componentInfo()

	# Call relativePosition
	print(info.relativePosition(0))

	# Create a sample Quat and call setRotation
	quat = Quat(0, 0, 0, 0)
	info.setRotation(0, quat)
	print(info.rotation(0))

	# Call hasParent
	print(info.hasParent(0))

Output:

.. testoutput:: CallMethods

	[0,0,0]
	[0,0,0,0]
	True


**Example 3 - Retrieving a List of Child Components from a ComponentInfo Object:**
The ``children()`` method does not take in any parameters.
The method returns a list of integers representing the child components.
The returned list can then be indexed into to obtain a specific component.

.. testcode:: CallChildrenMethod

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the ComponentInfo object
	info = ws.componentInfo()

	# Get a list of the child components
	childComponents = info.children(0)
	print(len(childComponents))
	print(childComponents)

Output:

.. testoutput:: CallChildrenMethod

	0
	[]


*bases:* :py:obj:`mantid.geometry.ComponentInfo`

.. module:`mantid.geometry`

.. autoclass:: mantid.geometry.ComponentInfo
    :members:
    :undoc-members:
    :inherited-members:
