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

*bases:* :py:obj:`mantid.geometry.ComponentInfo`

.. module:`mantid.geometry`

.. autoclass:: mantid.geometry.ComponentInfo 
    :members:
    :undoc-members:
    :inherited-members: