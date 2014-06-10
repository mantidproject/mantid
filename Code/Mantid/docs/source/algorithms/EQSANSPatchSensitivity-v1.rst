.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Process a detector sensitivity workspace and patch the area defined by
the masked pixels in the provided *PatchWorkspace*.

This algorithm is usually set up by
`ComputeSensitivity <http://www.mantidproject.org/ComputeSensitivity>`_ and 
executed by
`SANSSensitivityCorrection <http://www.mantidproject.org/SANSSensitivityCorrection>`_.

The *InputWorkspace* is a pre-calculated sensitivity workspace, usually computed
using the
`SANSSensitivityCorrection <http://www.mantidproject.org/SANSSensitivityCorrection>`_
algorithm.
The *PatchWorkspace* is a workspace where the pixels to patch are marked as masked.
The actual data of the *PatchWorkspace* is not used, only the masking information.

We consider each tube of the detector of the *InputWorkspace*. For each masked pixel
along a given tube, that pixel's sensitivity (Y-value) is set to:

- [If *UseLinearRegression=False*] The average of the sensitivity of all the unmasked pixels along the same tube.

- [If *UseLinearRegression=True*] The value found for that position along the tube by a linear regression performed
  using all unmasked pixels along that tube.
   
The output workspace is the patched version of the input workspace.

.. categories::
