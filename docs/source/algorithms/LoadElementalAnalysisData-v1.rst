.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The LoadElementalAnalysisData algorithm loads data from .dat files for the run chosen into a single
:ref:`WorkspaceGroup <WorkspaceGroup>` containing a :ref:`Workspace <Workspace>` for each Detector identified. Each :ref:`Workspace <Workspace>` contains the identified spectra
results. The name of the :ref:`WorkspaceGroup <WorkspaceGroup>` depends on the setting of GroupWorkspace. The path of the directory where the .dat files are located is provided
as an output property.

The algorithm searches the Data Search Directories for .dat files for the run specified. File names must be in the format 'ral[run number].rooth[detector number, spectra number].dat'.
A run number and name for the GroupWorkspace must be provided. The algorithm will stop searching once it has found the directory containing the .dat files that match the search criteria.


.. categories::

.. sourcelink::
