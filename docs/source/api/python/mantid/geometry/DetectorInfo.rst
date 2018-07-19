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

*bases:* :py:obj:`mantid.geometry.DetectorInfo`

.. module:`mantid.geometry`

.. autoclass:: mantid.geometry.DetectorInfo 
    :members:
    :undoc-members:
    :inherited-members:

