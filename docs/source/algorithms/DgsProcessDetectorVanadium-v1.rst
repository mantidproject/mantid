.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is responsible for processing the detector vanadium in
the form required for the sample data normalisation in the convert to
energy transfer process. Parameters in italics are controlled by the
`instrument parameter file (IPF) <http://www.mantidproject.org/InstrumentParameterFile>`_
unless provided to the algorithm via a property manager. The mappings are given
below.

+----------------------+-----------------+
| Parameter            | IPF Mapping     |
+======================+=================+
| DetVanIntRangeLow    | wb-integr-min   |
+----------------------+-----------------+
| DetVanIntRangeHigh   | wb-integr-max   |
+----------------------+-----------------+

Parameters in italics with dashed perimeters are only controllable by
the IPF name given. All underlined parameters are fixed and not
controllable. If the input detector vanadium is in TOF units and that is
the requested units for integration, the :ref:`ConvertUnits <algm-ConvertUnits>`
algorithm does not run. The range parameters feeding into :ref:`Rebin <algm-Rebin>`
are used to make a single bin. The resulting integrated vanadium workspace can be
saved to a file using the reduction property manager with the boolean property
SaveProcessedDetVan.

Workflow
########

.. figure:: /images/DgsProcessDetectorVanadiumWorkflow.png
   :alt: DgsProcessDetectorVanadiumWorkflow.png

Usage
-----

.. warning::

    This algorithm is not really intented for use at the command line, but is used
    within :ref:`DgsReduction <algm-DgsReduction>`.

.. categories::

.. sourcelink::
