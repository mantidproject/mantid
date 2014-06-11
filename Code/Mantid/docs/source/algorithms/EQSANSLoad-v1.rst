.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Workflow algorithm that loads EQSANS event data and applies basic
corrections to the workspace. The workflow proceeds as follows:

1. Determine the sample-detector distance and store it in the logs.

2. Locate the appropriate instrument configuration file and process it according to the input properties.

3. Move the detector position to the correct sample-detector distance.

4. Determine the source slit size and store it in the logs.

5. Move the detector according to the correct beam center position, 
   which is either given as input to the algorithm or pre-determined and stored in the input *ReductionProperties*.

6. Apply a TOF correction to take into account frame correction and the neutron flight path using
   `EQSANSTofStructure <http://www.mantidproject.org/EQSANSTofStructure>`_. This step includes trimming the edges
   of the TOF distribution according to the input properties or the instrument configuration file.

7. Convert TOF into wavelength.

8. Rebin the data according to the input *WavelengthStep*.


.. categories::
