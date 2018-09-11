.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm gathers the information needed to launch the LR reduction. It is called by the 
post-processing system after a new data file is created.

If a reduction template file is provided, the reduction will use the options in that file.
If no reduction template is provided, reduction parameters will be read from the Nexus file.

For sample data runs, the :ref:`LiquidsReflectometryReduction <algm-LiquidsReflectometryReduction>` 
algorithm is called to perform the reduction, followed by the 
:ref:`LRReflectivityOutput <algm-LRReflectivityOutput>` algorithm to put the different pieces of 
the reduced data into a single reflectivity file.

For direct beams, the :ref:`LRDirectBeamSort <algm-LRDirectBeamSort>` workflow algorithm is 
called to generate a scaling factor file. 

.. categories::

.. sourcelink::
