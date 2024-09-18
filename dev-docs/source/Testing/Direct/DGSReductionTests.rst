DGS Reduction Testing
=====================

.. contents::
   :local:

Set up
------

- Ensure you have the `MLZ Sample Data <http://download.mantidproject.org>`__ available on your machine.
- Open ``Interfaces`` -> ``Direct`` -> ``DGS Reduction``.
- Click on ``Tools`` -> ``Choose instrument``.
- Set ``Facility`` to ``MLZ`` and ``Instrument`` to ``TOFTOF``.
- ``TOFTOF Reduction`` interface should appear on the screen.

Data search directory
---------------------

- Specify path to the folder with Sample Data.

Inputs section
--------------

- In ``Vanadium runs`` enter ``12:14``.
- In ``Van. comment`` enter ``Van_res``. Keep ``EC factor`` to be ``1.000``. Leave ``T (K)`` empty.
- In ``Empty can runs`` enter ``15:17``. Change ``EC factor`` to ``0.9``. Leave ``T (K)`` empty.

Binning section
---------------

- ``Energy`` and ``Q`` should be ``on``.
- Set Energy ``start`` value to ``-6.000``, ``step`` to ``0.01``, and ``end`` to ``1.8``.
- Set Q ``start`` value to ``0.400``, ``step`` to ``0.1``, and end to ``2``.

Options section
---------------

- Check ``Subtract empty can from vanadium``.
- Normalise ``to monitor``.
- Correct TOF ``vanadium``.

Data table section
------------------

- First row: Type ``27:29`` in ``Data runs`` column and ``H2O_21C`` in ``Comment``.
- Second row (will appear once some input is provided in the first row): Type ``30:31`` in ``Data runs`` column and ``H2O_34C`` in ``Comment``.

Data reduction
--------------

- Click ``Reduce`` button to start the data reduction.
- 4 workspace groups and 2 workspaces will appear in the ``Workspaces`` panel of Workbench. If default workspace prefix ``ws`` was used, the final results can be found in the workspace group called ``gwsDataSQW``, under workspaces named ``ws_H2O_21C_sqw`` and ``ws_H2O_34C_sqw``.

Visualise results
-----------------

- Select e.g. the workspace ``ws_H2O_21C_sqw`` in ``gwsDataSQW`` workspace group.
- To plot the data, click the right mouse button and select ``Show Slice Viewer``.

Save reduction parameters
-------------------------

- Return to the TOFTOF Reduction GUI.
- Click on ``File`` -> ``Save As`` -> ``TOFTOF_reduction``. This will save the parameters of data reduction to a ``TOFTOF_reduction.xml`` file.
- Click on ``File`` -> ``New Reduction`` to remove the selected parameters.
- Click on ``File`` -> ``Open`` -> proceed to the folder with the saved ``xml`` file -> Select ``TOFTOF_reduction.xml``. This should load the pre-saved parameters.
- Click ``Reduce`` button to re-do the data reduction.