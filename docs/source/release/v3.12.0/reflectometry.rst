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
- The Python code generated when you tick `Output Notebook` has been improved to support special characters (e.g. `+`) in workspace names. Output workspaces are now set using the output properties of the algorithm rather than by variable assignment. This avoids the possibility of invalid characters being used in Python variable names.

  
Bug fixes
#########

- Fixed some bugs where transmission runs entered on the Settings tab were not being found, whether entered as a run number to load or as the name of an existing workspace in the ADS.
- The Python code generated when you tick `Output Notebook` has been changed so that all algorithm property values are enclosed in quotes. Unquoted values were causing failures in some algorithms. A bug has also been fixed in setting the legend location for the 4th (stitched) plot, which is shown when post-processing is performed.


Algorithms
----------
    
New features
############

- The new algorithm :ref:`algm-LoadILLPolarizationFactors` can load the polarization efficiency files used on D17 at ILL.
- The new algorithm :ref:`algm-MRInspectData` takes in raw event data and determines reduction parameters.

  
Improvements
############

- Removed the ``RegionOfDirectBeam`` property from :ref:`algm-ReflectometryReductionOne` and :ref:`algm-ReflectometryReductionOneAuto` because this is not used.
- Improvements to :ref:`algm-LoadILLReflectometry`:
    - Figaro NeXus files are now properly handled.
    - A new property, *BeamCentre* allows user to manually specify the beam position on the detector.
    - The *BeamPosition* property was renamed to *DirectBeamPosition* to better reflect its usage.
    - The *BraggAngle* property of :ref:`algm-LoadILLReflectometry` now works as expected: the detector will be rotated such that the reflected peak on the detector will be at twice *BraggAngle*.

Bug fixes
#########


:ref:`Release 3.12.0 <v3.12.0>`
