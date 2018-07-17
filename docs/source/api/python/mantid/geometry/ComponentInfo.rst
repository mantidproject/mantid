===============
 ComponentInfo
===============

This is a python binding to the C++ class Mantid::Geometry::ComponentInfo.

--------
Purpose
--------
The purpose of the ComponentInfo object is to allow the user to access information about the components which are part of an instrument. The ComponentInfo object can be used to access information such as the number of components in the instrument, the absolute position of a component as well as the absolute rotation of a component. There are many other methods available as well.

Many users may need this extra information so that they can have a better understanding of the instrument they are using and the components that make up the instrument - e.g. detectors. This extra information is also easy and fast to access meaning the users can make improvements to their experimental design with ease.

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