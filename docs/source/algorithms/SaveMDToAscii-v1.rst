
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves an :py:obj:`MDHistoWorkspace <mantid.dataobjects.MDHistoWorkspace>` to a plain ASCII
text file, with one row per bin. Each row contains, in order, the bin-centre coordinate for every
dimension written out, followed by the Intensity and the Error for that bin.

An integrated dimension is one with exactly one bin (``getNBins() == 1``). By default such dimensions are
excluded from the output columns, since their coordinate is constant across the whole workspace; set
``ExcludeIntegratedDimensions`` to ``False`` to include them instead, as constant-value columns. If every
dimension is integrated and ``ExcludeIntegratedDimensions`` is left as ``True``, the algorithm fails since
there would be no dimension columns left to write.

Normalization defaults to whatever the workspace itself declares via its display normalization
(:py:obj:`MDNormalization <mantid.api.MDNormalization>`, as set for example by :ref:`algm-BinMD` or
:ref:`algm-ConvertToMD`). Set ``Normalization`` explicitly to override the workspace's own setting:
``NumEventsNormalization`` divides Intensity and Error by the number of events in each bin, and
``VolumeNormalization`` multiplies Intensity and Error by the workspace's inverse bin volume.

``Separator`` and ``CustomSeparator`` control the column delimiter used in the output file.
``Precision`` controls the number of digits after the decimal point in scientific notation used for the
numeric columns (Intensity, Error, and the dimension coordinates).

If ``ExtraHeader`` is set to a non-empty string, it is written at the top of the file's header, above the
column names and shape lines. Every line is prefixed with ``#``, the same as the rest of the header; if
``ExtraHeader`` itself spans multiple lines, each one is prefixed individually.

Usage
-----

.. testcode:: SaveMDToAscii

    import os
    signalInput = [i for i in range(1, 10)]
    errorInput = [1 for i in range(1, 10)]

    ws = CreateMDHistoWorkspace(SignalInput=signalInput, ErrorInput=errorInput, Dimensionality='2',
                                Extents='-1,1,-1,1', NumberOfBins='3,3', Names='A,B', Units='U,V')

    savefile = os.path.join(config["defaultsave.directory"], "mdws_ascii.dat")
    SaveMDToAscii(InputWorkspace=ws, Filename=savefile, Separator='Space', Precision=3)

    print("File created: {}".format(os.path.exists(savefile)))

.. testoutput:: SaveMDToAscii

    File created: True

.. testcleanup:: SaveMDToAscii

    os.remove(savefile)

.. categories::

.. sourcelink::
