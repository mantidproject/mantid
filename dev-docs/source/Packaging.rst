=========
Packaging
=========

.. contents::
  :local:

This page gives an overview of the different packaging mechanisms used to deliver
Mantid to users.

Conda
-----

Mantid provides packages to be consumed by the `Conda <conda_>`_ package manager.
Even though it is a single codebase the repository is split across four conda
packages for users, and a metapackage used to create a development environment:

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
- ``mantid-developer``: The metapackage used to create a development environment. It contains
  all the required packages for building and testing our software. See :ref:`GettingStarted`
  for more detail on how to setup a development environment.

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

.. _build_packages_from_branch:

Build packages from branch using Jenkins
----------------------------------------

Developers can build packages to test branches using the ``build_packages_from_branch`` `Jenkins job <build_packages_from_branch_job_>`_. This job provides the ability to,

- Run system tests on Windows, Mac, Linux, or all three.
- Build a packages on Windows, Mac, Linux, or all three.
- Publish the package(s) to a given Anaconda channel and label.
- Publish the package(s) to a given Github repository under a specified tag.

for a given branch of mantid. The branch can be from the main mantid repo or from a remote.

Options
#######

- ``BUILD_DEVEL`` [``none``, ``all``, ``linux-64``, ``win-64``, ``osx-64``]: Run the system tests for this OS.
- ``BUILD_PACKAGE`` [``none``, ``all``, ``linux-64``, ``win-64``, ``osx-64``]: Build a package on this OS.
- ``PACKAGE_SUFFIX``: String to append onto the standalone package name, useful for distinguishing builds. By default this is ``Unstable``.
- ``PUBLISH_TO_ANACONDA``: Set true to publish to the given Anaconda channel and label.
- ``PUBLISH_TO_GITHUB``: Set true to publish to the Github repository using the specified tag.
- ``ANACONDA_CHANNEL``: Anaconda channel to upload the package to. By default this is ``mantid``.
- ``ANACONDA_CHANNEL_LABEL``: Label attached to the uploaded package. By default this is ``unstable``.
- ``GITHUB_RELEASES_REPO``: Repository to store the release. By Default this is ``mantidproject/mantid``.
- ``GITHUB_RELEASES_TAG``: Name of the tag for the release; only to be used for release candidate builds.
- ``ANACONDA_TOKEN_CREDENTIAL_ID`` [``anaconda-cloud-token``, ``anaconda-token-ornl``]: One of two credentials to use for publishing to Anaconda.
- ``GH_ORGANIZATION_OR_USERNAME``: Name of the organisation or Github user name who owns the repository with the code to build. By default this is ``mantidproject``, if you are building from a fork this will need to change to your username.
- ``BRANCH_NAME``: Name of the branch to build the packages from.

Example
#######

Say I've implemented a new file searching method on a branch ``1234_new_file_search`` and I want to test this on IDAaaS, one of the easiest ways to do this would be to build the packages and upload them to Anaconda using the pipeline. These are the steps I'd take to do this.

1. Go to the ``build_packages_from_branch`` `Jenkins job <build_packages_from_branch_job_>`_.
2. If needed click ``login`` in the top right of the window.
3. Go to ``Build with parameters`` in the side bar.
4. Fill out the following options:

   - ``BUILD_DEVEL`` = ``none``
   - ``BUILD_PACKAGE`` = ``linux-64``
   - ``PUBLISH_TO_ANACONDA`` = true
   - ``ANACONDA_CHANNEL_LABEL`` = ``new_file_system_test``
   - ``ANACONDA_TOKEN_CREDENTIAL_ID`` = ``anaconda-cloud-token``
   - ``BRANCH_NAME`` = ``1234_new_file_search``

5. Click ``Build``. This will take you back to the main job page, the build just set off will be the most recent (highest number) build on the left hand side. It is a good idea to make note of the build number / copy the link somewhere safe. If the build is for testing a pr, make sure to add the link to the testing instructions.
6. Once the job has successfully completed, check `the Mantid Anaconda page <mantid-conda-org_>`_ to make sure it has uploaded.
7. Head to IDAaaS (or any linux system) and run ``mamba install -c mantid/label/new_file_system_test mantidworkbench`` in a new environment to install the test package.

Most often, you won't need to upload the packages to Anaconda, this is most useful in cases where installing standalone packages is inconvenient. Standalone package builds created by the jenkins job can be found under the jenkins job build artifacts, this is near the top of the page. Say you built a package for Windows using the jenkins job, you should find a ``mantidworkbench`` exe file in the build artifacts.

.. _build_custom_mantid-developer:

Building a custom ``mantid-developer`` environment
--------------------------------------------------

This is useful if you need to change a pinned version of one of Mantid's dependencies and test the change locally.

1. Create a conda environment and install ``boa`` and ``versioningit`` into it. For this example, called ``mantid_dev_builder``.
2. Make your changes to the conda recipe files.
3. Change directory to ``mantid/conda/recipes``
4. With ``mantid_dev_builder`` active, run ``conda mambabuild ./mantid-developer/``. This will build a local version of ``mantid-developer`` with your changes and place it in ``mantid_dev_builder``'s ``conda-bld`` folder. The output from ``conda mambabuild`` should tell you the location.
5. Deactivate ``mantid_dev_builder`` and create a new environment to install the custom ``mantid-developer`` package into (e.g if you were testing a new version of numpy you might call it ``mantid_dev_numpy_test``)
6. ``mamba install -c <path to mantid_dev_builder's conda-bld folder> mantid-developer`` to install the package.
7. You will need to re-run cmake with this new environment.

Note: If you have ``boa`` installed in your base environment it seems ``conda mambabuild`` will use it over your activated environment. In this case you will likely get an error that you don't have ``versioningit`` installed. One way to fix this is to install ``versioningit`` into your base environment and just use that instead of making a new environment.


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
.. _build_packages_from_branch_job: https://builds.mantidproject.org/job/build_packages_from_branch/
