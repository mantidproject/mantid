.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that wraps
:ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` to perform a double loop over files
with chunks, accumulating the result. The additional properties from
what is allowed in :ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>` is for how the file
is loaded (e.g. ``MaxChunkSize`` and ``FilterBadPulses``), an optional
``Characterizations`` :class:`table <mantid.api.ITableWorkspace>`, and
an optional ``AbsorptionWorkspace``. The ``AbsorptionWorkspace``
should be in units of ``Wavelength`` for every detector pixel and is
divided from the chunk after loaded. For all other information, see
:ref:`AlignAndFocusPowder <algm-AlignAndFocusPowder>`.

In broad terms, this algorithm is the pseudo-code:

.. code-block:: python

   for filename in filenames:
       cachefile = CreateCacheFilename(filename, ...)
       if cachefile exists:
           wksp_single = LoadNexusProcessed(cachefile)
       else:
           for chunk in DetermineChunking(filename):
               wksp_single_chunk = Load(chunk)
               wksp_single_chunk = AlignAndFocusPowder(wksp_single_chunk, ...)
           # accumulate chunks into wksp_single
           SaveNexusProcess(wksp_single, cachefile)
       # accumulate data from files into OutputWorkspace

Algorithms used by this are:

#. :ref:`algm-AlignAndFocusPowder-v1`
#. :ref:`algm-CompressEvents-v1`
#. :ref:`algm-ConvertUnits-v1`
#. :ref:`algm-CreateCacheFilename-v1`
#. :ref:`algm-DeleteWorkspace-v1`
#. :ref:`algm-DetermineChunking-v1`
#. :ref:`algm-Divide-v1`
#. :ref:`algm-EditInstrumentGeometry-v1`
#. :ref:`algm-FilterBadPulses-v1`
#. :ref:`algm-Load-v1`
#. :ref:`algm-LoadNexusProcessed-v1`
#. :ref:`algm-PDDetermineCharacterizations-v1`
#. :ref:`algm-Plus-v1`
#. :ref:`algm-RenameWorkspace-v1`
#. :ref:`algm-SaveNexusProcessed-v1`


.. categories::

.. sourcelink::
