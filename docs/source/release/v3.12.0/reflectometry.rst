=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

    
ISIS Reflectometry Interface
----------------------------

New features
############


Improvements
############

- Menu items and toolbar buttons are now enabled/disabled when appropriate, e.g. to prevent table modification during processing. Directly editing table rows is also disabled during processing.
- Removed the 'DirectBeam' box from the settings tab of the ISIS Reflectometry interface because this is not used.
- Properties on the Runs tab now take precedence over properties on the Settings tab.
- Output workspace names have been improved. Names now use '+' to indicate preprocessed (i.e. summed) workspaces, rather than '_', which is used to indicate postprocessed (i.e. stitched) workspaces.

  
Bug fixes
#########

- Fixed some bugs where transmission runs entered on the Settings tab were not being found, whether entered as a run number to load or as the name of an existing workspace in the ADS.


Algorithms
----------
    
New features
############

- The new algorithm :ref:`algm-LoadILLPolarizationFactors` can load the polarization efficiency files used on D17 at ILL.
- The new algorithm :ref:`algm-MRInspectData` takes in raw event data and determines reduction parameters.

  
Improvements
############

- Removed the ``RegionOfDirectBeam`` property from :ref:`algm-ReflectometryReductionOne` and :ref:`algm-ReflectometryReductionOneAuto` because this is not used.

  
Bug fixes
#########

- The *BraggAngle* property of :ref:`algm-LoadILLReflectometry` now works as expected: the detector will be rotated such that the reflected peak will be at twice *BraggAngle*.


:ref:`Release 3.12.0 <v3.12.0>`
