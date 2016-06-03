.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Workflow algorithm that loads SWANS event data and applies basic
corrections to the workspace. The workflow proceeds as follows:

1. Determine the source slit size and store it in the logs.

2. Move the detector according to the correct beam center position, 
   which is either given as input to the algorithm or pre-determined and stored in the input *ReductionProperties*.

3. Convert TOF into wavelength.

4. Rebin the data according to the input *WavelengthStep*.

This algorithm is rarely called directly. It is called by 
:ref:`SANSReduction <algm-SANSReduction>`.

.. categories::

.. sourcelink::
