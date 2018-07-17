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

*bases:* :py:obj:`mantid.api.SpectrumInfo`

.. module:`mantid.api`

.. autoclass:: mantid.api.SpectrumInfo 
    :members:
    :undoc-members:
    :inherited-members:

