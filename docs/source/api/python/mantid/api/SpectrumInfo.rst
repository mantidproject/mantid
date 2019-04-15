==============
 SpectrumInfo
==============

This is a python binding to the C++ class Mantid::API::SpectrumInfo.

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
	from mantid.api import SpectrumInfo
	print("The type is SpectrumInfo: {}".format(isinstance(info, SpectrumInfo)))

Output:

.. testoutput:: CreateSpectrumInfoObject

	The type is SpectrumInfo: True


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


**Example 3 - Calling Some Methods on the SpectrumInfo Object:**
This example shows how to call a few different methods on the SpectrumInfo object.

The ``l1()`` method does not take in any parameters and returns the distance from the source to the sample.
The return value is a float.

The ``sourcePosition()`` method does not take any parameters and returns the absolute source position.
The return value is a ``V3D`` object which is a point in 3D space.

The ``getSpectrumDefinition()`` method takes in an integer ``index`` parameter and returns a ``SpectrumDefinition`` object.
The returned object can then be used to call other methods that belong to ``SpectrumDefinition``.

.. testcode:: CallMethods

	# Create a workspace to use
	ws = CreateSampleWorkspace()

	# Get the SpectrumInfo object
	info = ws.spectrumInfo()

	# Call l1
	print(info.l1())

	# Call sourcePosition
	print(info.sourcePosition())

	# Get a SpectrumDefinition object
	spectrumDefinition = info.getSpectrumDefinition(0)
	print(type(spectrumDefinition))

Output:

.. testoutput:: CallMethods

	10.0
	[0,0,-10]
	<class '_api.SpectrumDefinition'>


*bases:* :py:obj:`mantid.api.SpectrumInfo`

.. module:`mantid.api`

.. autoclass:: mantid.api.SpectrumInfo
    :members:
    :undoc-members:
    :inherited-members:

