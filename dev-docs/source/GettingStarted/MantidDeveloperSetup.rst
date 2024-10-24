Create a Conda environment and install the ``mantid-developer`` Conda metapackage by following the steps below:

* First create and activate a new conda environment. Here we have named it ``mantid-developer`` for consistency with the rest of the developer documentation but you are free to choose any name.

  .. code-block:: sh

    mamba create -n mantid-developer
    mamba activate mantid-developer

* Install the ``mantid-developer`` Conda metapackage from the ``mantid`` Conda channel. You will normally want the nightly version.

  .. code-block:: sh

    mamba install -c mantid/label/nightly mantid-developer
