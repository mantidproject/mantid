Create ``mantid-developer`` conda environment by following the steps below:

* First create a new conda environment and install the ``mantid-developer`` conda metapackage.
  You will normally want the nightly version, specified below by adding the label ``mantid/label/nightly``.
  Here we have named the conda environment ``mantid-developer`` for consistency with the rest of the documentation
  but you are free to choose any name. The ``neutrons`` channel is also required for PyStoG.

  .. code-block:: sh

    mamba create -n mantid-developer mantid/label/nightly::mantid-developer -c neutrons

* Then activate the conda environment with

  .. code-block:: sh

    mamba activate mantid-developer

* It is important that you regularly update your ``mantid-developer`` environment so that the dependencies are consistent with those used in production.
  With your ``mantid-developer`` environment activated, run the following command:

  .. code-block:: sh

    mamba update -c mantid/label/nightly -c neutrons --all

See :ref:`packaging <Packaging>` for more information on this topic.
