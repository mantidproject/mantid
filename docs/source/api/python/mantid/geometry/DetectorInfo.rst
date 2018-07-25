==============
 DetectorInfo
==============

This is a python binding to the C++ class Mantid::Geometry::DetectorInfo.

--------
Purpose
--------
The purpose of the DetectorInfo object is to allow the user to access information about the detector(s) being used in an experiment. The DetectorInfo object can be used to access geometric information such as the number of detectors in the beamline, the absolute position of a detector as well as the absolute rotation of a detector.

Many users may need this extra information so that they can have a better understanding of the beamline they are using. This extra information is also easy and fast to access meaning the users can make improvements to their experimental design with ease.

DetectorInfo is one of three objects that the user can gain access to from a Workspace. 
The other two are:
  * SpectrumInfo
  * ComponentInfo

-------
Usage
-------

**Example 1 - Creating a DetectorInfo Object:**
This example shows how to obtain a DetectorInfo object from a workspace object.
The return value is a ``DetectorInfo`` object.

.. testcode:: CreateDetectorInfoObject
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the DetectorInfo object
	info = ws.detectorInfo()
	print(type(info))

**Example 2 - Calling the setMasked method on the DetectorInfo Object:**
This example shows how to call the ``setMasked`` method.
The method takes in an integer ``index`` parameter which corresponds to a component and a boolean ``masked`` parameter which allows the user to set the masking to True or False.

.. testcode:: CallSetMaskedMethod

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the DetectorInfo object
	info = ws.detectorInfo()

	# Call setMasked
	info.setMasked(0, True)
	print(info.isMasked(0))
	info.setMasked(0, False)
	print(info.isMasked(0))

**Example 3 - Calling the twoTheta method on the DetectorInfo Object:**
The ``twoTheta()`` method takes in an integer ``index`` parameter which represents a detector index. 
The return value is a float which represents the scattering angle with respect to the beam direction.

.. testcode:: CallTwoThetaMethod
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the DetectorInfo object
	info = ws.detectorInfo()

	# Call twoTheta
	print(type(info.twoTheta(0)))

**Example 4 - Calling the position method on the DetectorInfo Object:**
The ``position()`` method takes an ``index`` parameter which represents a detector index. 
The method returns the absolute position of that detector. 
The returned object is of type V3D which is a position in 3D space.

.. testcode:: CallPositionMethod
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the DetectorInfo object
	info = ws.detectorInfo()

	# Call the position method
	print(type(info.position(0)))


**Example 5 - Calling the size method on the DetectorInfo Object:**
The ``size()`` method does not take in any parameters and returns a number of detectors in the instrument. 
One can also use the built in ``__len__`` function to obtain the same result.

.. testcode:: CallSizeAndLenMethods
	
	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the DetectorInfo object
	info = ws.detectorInfo()

	# Call size and __len__
	print(info.size())
	print(len(info))

Output:

.. testoutput:: CreateDetectorInfoObject

	<class 'mantid.geometry._geometry.DetectorInfo'>

.. testoutput:: CallSetMaskedMethod

	True
	False

.. testoutput:: CallTwoThetaMethod

	<class 'float'>

.. testoutput:: CallPositionMethod

	<class 'mantid.kernel._kernel.V3D'>

.. testoutput:: CallSizeAndLenMethods

	200
	200


*bases:* :py:obj:`mantid.geometry.DetectorInfo`

.. module:`mantid.geometry`

.. autoclass:: mantid.geometry.DetectorInfo 
    :members:
    :undoc-members:
    :inherited-members:

