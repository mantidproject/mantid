===============
 ComponentInfo
===============

This is a python binding to the C++ class Mantid::Geometry::ComponentInfo.

*bases:* :py:obj:`mantid.geometry.ComponentInfo`

:py:obj:`~mantid.geometry.ComponentInfo` provides faster and simpler access to instrument/beamline geometry as required by
Mantid :py:obj:`algorithm <mantid.api.Algorithm>` than was possible using :py:obj:`Instrument <mantid.geometry.Instrument>`.
:py:obj:`~mantid.geometry.ComponentInfo` and :py:obj:`DetectorInfo <mantid.geometry.DetectorInfo>` are designed as full replacements to :py:obj:`Instrument <mantid.geometry.Instrument>`.

Most of the information concerning :py:obj:`~mantid.geometry.ComponentInfo` can be found in the :ref:`Instrument Access Layers <InstrumentAccessLayers>` document, which provides details on how :py:obj:`~mantid.geometry.DetectorInfo` interacts with other geometry access layers.

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


**Example 4 - Print indices of detectors in "bank1" that are masked**

.. testcode:: show_masked_detectors_in_bank

    from mantid.simpleapi import CreateSampleWorkspace

    ws = CreateSampleWorkspace()
    comp_info = ws.componentInfo()
    det_info = ws.detectorInfo()
    det_info.setMasked(2, True) # Mask  a bank 1 detector for demo
    det_info.setMasked(len(det_info)-1, True) # Mask detector not in bank 1
    bank_index = comp_info.indexOfAny('bank1')
    for det_index in comp_info.detectorsInSubtree(bank_index):
        if det_info.isMasked(int(det_index)):
            print('Masked detector index of bank1 is {}'.format(det_index))


Output:

.. testoutput:: show_masked_detectors_in_bank

   Masked detector index of bank1 is 2


Reference
---------

.. module:`mantid.geometry`

.. autoclass:: mantid.geometry.ComponentInfo
    :members:
    :undoc-members:
    :inherited-members:
