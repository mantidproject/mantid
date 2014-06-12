.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is the top-level workflow algorithm for direct geometry
spectrometer data reduction. This algorithm is responsible for gathering
the necessary parameters and generating calls to other workflow or
standard algorithms.

Workflow
########

Parameters for the child algorithms are not shown due to sheer number.
They will be detailed in the child algorithm diagrams. Items in
parallelograms are output workspaces from their respective algorithms.
Not all output workspaces are subsequently used by other algorithms.

.. figure:: /images/DgsReductionWorkflow.png
   :alt: DgsReductionWorkflow.png

   DgsReductionWorkflow.png

.. categories::
