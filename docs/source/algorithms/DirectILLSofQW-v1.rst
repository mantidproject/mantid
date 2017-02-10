.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm transfers the output of the :ref:`DirectILLReduction <algm-DirectILLReduction>` algorithm to :math:`S(q,\omega)`. The :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>` algorithm is used as the backend.

.. note::
  *InputWorkspace* must have **spectrum numbers** as the vertical axis. When running :ref:`DirectILLReduction <algm-DirectILLReduction>`, use `VerticalAxisUnits='Spectrum Number'`.

.. note::
  *InputWorkspace* must have equidistant energy binning. Use the energy rebinning parameters in :ref:`DirectILLReduction <algm-DirectILLReduction>`.

q binning
#########

There are two binning modes for :math::`q` selectable by *QBinningMode*:

'Binning Median 2Theta'
  calculates the median scattering angle over *InputWorkspace* and the corresponding bin width and :math:`q` range.

'Binning Manual'
  Lets the user specify the binnin parameters by *QBinninParams*. The value of this property is forwarded to :ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>`

Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
