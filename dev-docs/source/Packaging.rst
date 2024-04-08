=========
Packaging
=========

.. contents::
  :local:

This page gives an overview of the different packaging mechansims used to deliver
Mantid to users.

Conda
-----

Mantid provides packages to be consumed by the `Conda <conda_>`_ package manager.
This is separate to the ``mantid-developer`` environment files that setup a local
development environment. See :ref:`GettingStarted` for more detail on how to setup a development environment.
The packages described here are those deployed for users to install.

Even though it is a single codebase the repository is split across 4 conda
packages:

- ``mantid``: Provides Python access to the non-GUI elements of mantid and allows
  users to import mantid as a Python library for use in other programs.
  This package is the base package that can be installed on its own without the requirement
  for any other packages to be installed.
- ``mantidqt``: Provides Python access to Qt-widgets library customized for use
  with the mantid framework, e.g. instrumentview, sliceviewer, file finder widget etc.
- ``mantiddocs``: Provides the built-in help pages for the Qt Help viewer in
  workbench.
- ``mantidworkbench``: The graphical application produced by the Mantid project that
  brings together all of the above packages.

Each package is built separately for ``x64`` Windows, Linux and macOS with the
exception of ``mantiddocs`` where a single, ``noarch`` package is created for all
operating systems as it only contains static HTML.

All mantid-conda packages are available to users through Mantid's
`Conda organization <mantid-conda-org_>`_.

.. _building-conda-packages:

Building Conda Packages
#######################

Conda requires `recipes <conda-recipes-docs_>`_ as input for producing packages.
The recipes are stored in the `main codebase <mantid-conda-recipes_>`_ so that
they can evolve as the code develops. The packages are built as part of the
`CI pipeline <ci-pipeline_>`_ using a `script <package-conda_>`_ to encapsulate
the steps.

To build the packages locally it is recommended that you use a separate
clone of the repository as the build copies the repository content to a temporary
working directory and any additional build directories can interfere with the
packaging. The following steps should produce a set of conda packages in a ``conda-bld``
directory relative to the current working directory
(on Windows run this from Git bash):

.. code:: sh

   git clone https://github.com/mantidproject/mantid.git mantid-conda-build
   mantid-conda-build/buildconfig/Jenkins/Conda/package-conda $PWD \
     --build-mantid --build-qt --build-docs --build-workbench 2>&1 | tee package.log
   # wait quite a while ...
   # packages will appear in a conda-bld directory

You can build the workbench without the documentation by:

.. code:: sh

   mantid-conda-build/buildconfig/Jenkins/Conda/package-conda $PWD \
     --build-mantid --build-qt --build-workbench-without-docs 2>&1 | tee package.log

To create a new test environment with packages from the local build:

.. code:: sh

   mamba create -n local-package-test -c $PWD/conda-bld mantidworkbench
   mamba activate local-package-test
   workbench


Standalone
----------

Mantid also provides a set of bundled packages that provide a complete install of
``MantidWorkbench`` and all of its dependencies without a user having to
first setup conda and then install the relevant conda packages.
The standalone packages are available to users through Mantid's `download page <download-page_>`_.

Each installer is simply a conda environment, with ``mantidworkbench`` installed within
it followed by the appropriate installer technology wrapped around it.
Installers are provided for:

- `Windows`: A `NSIS <nsis_>`_ installer package providing a wizard-style interface
  for installing/uninstalling MantidWorkbench.
- `macOS`: A `disk image <dmg_>`_ providing a facility to drag-and-drop MantidWorkbench
  to the `/Applications` folder
- `Linux`: An ``xz``-compressed tarball of the package files and directories that
  can be extracted to any location.

Building Standalone Installers
##############################

The installers are built as part of the `CI pipeline <ci-pipeline_>`_ using a
`script <package-standalone_>`_ to encapsulate the build steps.

To build an installer locally first build a set of conda packages using the
:ref:`instructions above <building-conda-packages>`. Once you have the ``conda-bld``
directory run the following from the same working directory:

.. code:: sh

   mantid-conda-build/buildconfig/Jenkins/Conda/package-standalone \
     $PWD --package-suffix Unstable 2>&1 | tee standalone-package.log
   # wait some time and the installer will appear in the working directory

The ``--package-suffix`` argument is an optional string to append to the name
of the final package. We generally pick ``Unstable`` for installers not built
by the CI pipeline to indicate it has been built outside of the standard process.


.. _conda: https://conda.io
.. _mantid-conda-recipes: https://github.com/mantidproject/mantid/tree/main/conda
.. _mantid-conda-org: https://anaconda.org/mantid
.. _conda-recipes-docs: https://docs.conda.io/projects/conda-build/en/stable/concepts/recipe.html
.. _mantid-conda-recipes: https://github.com/mantidproject/mantid/tree/main/conda
.. _ci-pipeline: https://github.com/mantidproject/mantid/blob/main/buildconfig/Jenkins/Conda/nightly_build_and_deploy.jenkinsfile
.. _package-conda: https://github.com/mantidproject/mantid/blob/main/buildconfig/Jenkins/Conda/package-conda
.. _package-standalone: https://github.com/mantidproject/mantid/blob/main/buildconfig/Jenkins/Conda/package-standalone
.. _download-page: https://download.mantidproject.org
.. _nsis: https://sourceforge.net/projects/nsis/
.. _dmg: https://en.wikipedia.org/wiki/Apple_Disk_Image
.. _standalone-scripts: https://github.com/mantidproject/mantid/blob/main/installers/conda
