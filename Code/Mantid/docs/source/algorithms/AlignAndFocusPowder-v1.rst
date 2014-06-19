.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is a workflow algorithm that does the bulk of the work for time
focusing diffraction data. This is done by executing several
sub-algorithms as listed below.

#. :ref:`algm-RemovePromptPulse` (event workspace only)
#. :ref:`algm-CompressEvents` (event workspace only)
#. :ref:`algm-CropWorkspace`
#. :ref:`algm-MaskDetectors`
#. :ref:`algm-Rebin` or :ref:`algm-ResampleX` if not d-space binning
#. :ref:`algm-AlignDetectors`
#. If LRef, minwl, or DIFCref are specified:

   #. :ref:`algm-ConvertUnits` to time-of-flight
   #. :ref:`algm-UnwrapSNS`
   #. :ref:`algm-RemoveLowResTOF`
   #. :ref:`algm-ConvertUnits` to d-spacing

#. :ref:`algm-Rebin` if d-space binning
#. :ref:`algm-DiffractionFocussing`
#. :ref:`algm-SortEvents` (event workspace only)
#. :ref:`algm-EditInstrumentGeometry` (if appropriate)
#. :ref:`algm-ConvertUnits` to time-of-f








.. image:: /images/AlignAndFocusPowderFlowchart.png

.. categories::
