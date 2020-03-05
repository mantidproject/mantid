.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The workspace data are stored in the file in columns: the first column
contains the X-values, followed by pairs of Y and E values. Columns are
separated by commas. The resulting file can normally be loaded into a
workspace by the :ref:`algm-LoadAscii` algorithm.

As far as we are aware, this algorithm is only used by the ISIS Indirect group (including within the Indirect Diffraction GUI).
Please see the new version: :ref:`algm-SaveAscii`.

Limitations
###########

The algorithm assumes that the workspace has common X values for all
spectra (i.e. is not a :ref:`ragged workspace <Ragged_Workspace>`). Only
the X values from the first spectrum in the workspace are saved out.

.. categories::

.. sourcelink::
