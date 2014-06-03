.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm replicates the sequence of actions undertaken by
MuonAnalysis in order to produce a Muon workspace ready for fitting.

Specifically:

#. Load the specified filename
#. Apply dead time correction
#. Group the workspace
#. Offset, crop and rebin the workspace
#. If the loaded data is multi-period - apply the specified operation to
   specified periods to get a single data set.
#. Use :ref:`algm-MuonCalculateAsymmetry` to get the
   resulting workspace.

Workflow
########

.. figure:: /images/MuonWorkflow.png
   :alt: MuonWorkflow.png

   MuonWorkflow.png

.. categories::
