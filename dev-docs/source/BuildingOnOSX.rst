.. _BuildingOnOSX:

================
Building on OS X
================

.. toctree::
   :hidden:

   BuildingOnOSXHistoric

.. contents::
  :local:


The minimum supported version of macOS is High Sierra (10.13).

These instructions are from the assumptions of a blank newly installed version of High Sierra using Homebrew for dependency management.

#. Install Xcode

#. Install Apple's Command Line tools (required by Homebrew)

   .. code-block:: sh

      xcode-select --install

#. Install `Homebrew <http://brew.sh>`_.

   .. code-block:: sh

      ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

#. Add the necessary 'taps':

   In order to be able to 'tap' the ``mantidproject/mantid`` 'tap' we need to have a couple of packages installed

   .. code-block:: sh

      brew install git
      brew tap mantidproject/mantid
      brew tap homebrew/cask
      brew install --cask xquartz
      brew install --cask mactex
      brew install mantid-developer

#. Python is now keg-only. Add the appropriate version to ``PATH`` in shell profile and restart the terminal:

   .. code-block:: sh

      # Assume we are using bash
      echo 'export PATH="/usr/local/opt/python@3.8/bin:$PATH"' >> ~/.bash_profile

      # If you have enabled Zsh
      echo 'export PATH="/usr/local/opt/python@3.8/bin:$PATH"' >> ~/.zshenv

#. Downgrade setuptools to 48.0.0 until https://github.com/mantidproject/mantid/issues/29010 is fixed.

   .. code-block:: sh

      python3 -m pip install setuptools==48.0.0

#. Install python requirements

   .. code-block:: sh

      # Note this export assumes Intel, if you are using
      # M1 please contact us, as Pycifrw requires upstream work
      export ARCHFLAGS="-arch x86_64"

      python3 -m pip install -r /usr/local/Homebrew/Library/Taps/mantidproject/homebrew-mantid/requirements.txt


Instructions for Historic Versions
----------------------------------

See :ref:`BuildingOnOSXHistoric` for instructions for older versions of OSX.
