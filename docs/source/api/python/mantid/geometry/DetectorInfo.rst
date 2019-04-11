==============
 DetectorInfo
==============

This is a python binding to the C++ class Mantid::Geometry::DetectorInfo.

Most of the information concerning ``DetectorInfo`` can be found in the `Instrument Access Layers <https://github.com/mantidproject/mantid/blob/9e3d799d40fda4a5ca08887e8c47f41c3316da91/docs/source/concepts/InstrumentAccessLayers.rst>`_ document.

--------
Purpose
--------
The purpose of the ``DetectorInfo`` object is to allow the user to access information about the detector(s) being used in an experiment. The ``DetectorInfo`` object can be used to access geometric information such as the number of detectors in the beamline, the absolute position of a detector as well as the absolute rotation of a detector.

Many users may need this extra information so that they can have a better understanding of the beamline they are using. This information is easy and fast to access. Some information like mask flags can be modified directly.

The ``DetectorInfo`` object is one of three objects that the user can gain access to from a workspace.
The other two are:

 * SpectrumInfo
 * ComponentInfo

---------
Indexing
---------
The ``DetectorInfo`` object is accessed by an index going from 0 to N-1, where N is the number of detectors.
It is important to note that the detector index is NOT the detector ID. A detector index is a way of addressing and enumerating detectors in the beamline.
A detector index can be found from a detector ID using ``indexOf``.

-------
Usage
-------

**Example 1 - Creating a DetectorInfo Object:**
This example shows how to obtain a ``DetectorInfo`` object from a workspace object.
The return value is a ``DetectorInfo`` object.

.. testcode:: CreateDetectorInfoObject

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the DetectorInfo object
	info = ws.detectorInfo()
	print(type(info))

Output:

.. testoutput:: CreateDetectorInfoObject

	<class '_geometry.DetectorInfo'>


**Example 2 - Calling Some Methods on the DetectorInfo Object:**
This example shows how to call a few different methods on the DetectorInfo object.

The ``setMasked`` method takes in an integer ``index`` parameter which corresponds to a detector as well as a boolean ``masked`` parameter.
The user then has the option to set the masking of the detector identified by ``index`` to ``True`` or ``False``.

The ``twoTheta()`` method takes in an integer ``index`` parameter which represents a detector index.
The return value is a float which represents the scattering angle with respect to the beam direction.

The ``position()`` method takes an ``index`` parameter which represents a detector index.
The method returns the absolute position of that detector.
The returned object is of type ``V3D`` which is a point in 3D space.

The ``size()`` method does not take in any parameters and returns the number of detectors in the instrument.
One can also use the built in ``__len__`` function to obtain the same result.

.. testcode:: CallMethods

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the DetectorInfo object
	info = ws.detectorInfo()

	# Call setMasked
	info.setMasked(0, True)
	print(info.isMasked(0))
	info.setMasked(0, False)
	print(info.isMasked(0))

	# Call twoTheta
	print(info.twoTheta(0))

	# Call the position method
	print(info.position(0))

	# Call size and __len__
	print(info.size())
	print(len(info))

Output:

.. testoutput:: CallMethods

	True
	False
	0.0
	[0,0,5]
	200
	200


*bases:* :py:obj:`mantid.geometry.DetectorInfo`

.. module:`mantid.geometry`

.. autoclass:: mantid.geometry.DetectorInfo
    :members:
    :undoc-members:
    :inherited-members:

