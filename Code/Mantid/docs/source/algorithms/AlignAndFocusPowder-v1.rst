.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is a workflow algorithm that does the bulk of the work for time
focusing diffraction data. This is done by executing several
sub-algorithms as listed below.

#. :ref:`_algm-RemovePromptPulse` (event workspace only)
#. :ref:`_algm-CompressEvents` (event workspace only)
#. :ref:`_algm-CropWorkspace`
#. :ref:`_algm-MaskDetectors`
#. :ref:`_algm-Rebin` or :ref:`_algm-ResampleX` if not d-space binning
#. :ref:`_algm-AlignDetectors`
#. If LRef, minwl, or DIFCref are specified:

   #. :ref:`_algm-ConvertUnits` to time-of-flight
   #. :ref:`_algm-UnwrapSNS`
   #. :ref:`_algm-RemoveLowResTOF`
   #. :ref:`_algm-ConvertUnits` to d-spacing

#. :ref:`_algm-Rebin` if d-space binning
#. :ref:`_algm-DiffractionFocussing`
#. :ref:`_algm-SortEvents` (event workspace only)
#. :ref:`_algm-EditInstrumentGeometry` (if appropriate)
#. :ref:`_algm-ConvertUnits` to time-of-f

.. categories::
