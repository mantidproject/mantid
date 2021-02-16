.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that does the bulk of the work for time
focusing diffraction data. This is done by executing several
sub-algorithms as listed below.

#. :ref:`algm-CompressEvents` (event workspace only)
#. :ref:`algm-CropWorkspace`
#. :ref:`algm-RemovePromptPulse`
#. :ref:`algm-MaskDetectors`
#. :ref:`algm-Rebin` or :ref:`algm-ResampleX` if not d-space binning
#. :ref:`algm-AlignDetectors`
#. If ``LowResRef`` or ``CropWavelengthMin`` are specified:

   #. :ref:`algm-ConvertUnits` to time-of-flight
   #. :ref:`algm-UnwrapSNS`
   #. :ref:`algm-RemoveLowResTOF`
   #. :ref:`algm-ConvertUnits` to d-spacing

#. :ref:`algm-Rebin` if d-space binning (optionally :ref:`algm-RebinRagged` if ``DeltaRagged``)
#. :ref:`algm-DiffractionFocussing`
#. :ref:`algm-SortEvents` (event workspace only)
#. :ref:`algm-EditInstrumentGeometry` (if appropriate)
#. :ref:`algm-ConvertUnits` to time-of-flight
#. :ref:`algm-RebinRagged` (if ``DeltaRagged`` is specified) to bin each spectrum differently

Workflow
########

.. diagram:: AlignAndFocusPowder-v1_wkflw.dot

Usage
-----

**Example: A simple Powgen example**

The files needed for this example are not present in our standard usage data
download due to their size.  They can however be downloaded using these links:
`PG3_9830_event.nxs <https://github.com/mantidproject/systemtests/blob/master/Data/PG3_9830_event.nxs?raw=true>`_
and
`pg3_mantid_det.cal <https://testdata.mantidproject.org/ftp/external-data/MD5/e2b281817b76eadbc26a0a2617477e97>`_.

You will have to rename :literal:`pg3_mantid_det.cal` manually, as its name in the link above is a list of random characters.

.. code-block:: python

    PG3_9830_event = Load('PG3_9830_event.nxs')
    PG3_9830_event = AlignAndFocusPowder(PG3_9830_event,
        CalFileName='pg3_mantid_det.cal', Params='100')


.. categories::

.. sourcelink::
