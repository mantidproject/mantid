.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm provides bi-directional conversion between regular :ref:`PeaksWorkspace <PeaksWorkspace>`
and :ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>`.

When casting a PeaksWorkspace into a LeanElasticPeaksWorkspace, information related to detector
(DetID, BankName, Row, Col) is discarded.
However, other experiment information are retained in the LeanElasticPeaksWorkspace, including
run, hkl, goniometer, sample, as well as the corresponding information at the individual Peak level.

When casting a LeanElasticPeaksWorkspace into a PeaksWorkspace, a donor PeaksWorkspace with proper
instrument configuration is required.
Incorrect goniometer setting will lead to a Peak with negative wavelength, which will be excluded from the output
PeaksWorkspace.

Usage
-----

1) PeaksWorkspace to LeanElasticPeaksWorkspace

.. code-block:: python

    # pws is a regular PeaksWorkspace available in memroy
    ConvertPeaksWorkspace(PeakWorkspace='pws', OutputWorkspace='lpws')

2) LeanElasticPeaksWorkspace to PeaksWorkspace

.. code-block:: python

    ConvertPeaksWorkspace(
        PeakWorkspace='lpws',            # a LeanElasticPeaksWorkspace in memory
        InstrumentWorkpace="donor_pws",  # a PeaksWorkspace with correct Goniometer setting and other exp info
        OutputWorkspace='pws',           # output
        )

.. categories::

.. sourcelink::
