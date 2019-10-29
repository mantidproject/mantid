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

1. Install Xcode 10.1

2. Install Apple's Command Line tools (required by Homebrew)

.. code-block:: sh

   xcode-select --install

3. Install `Homebrew <http://brew.sh>`_.

.. code-block:: sh

   ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

4. Add the necessary 'taps'. The last 4 are to use qt4.

In order to be able to 'tap' the ``mantidproject/mantid`` 'tap' we need to have a couple of packages installed

.. code-block:: sh

   brew install git
   brew tap homebrew/science
   brew tap mantidproject/mantid
   brew tap caskroom/cask
   brew tap cartr/qt4
   brew tap-pin cartr/qt4
   brew cask install xquartz
   brew cask install mactex

5. Reset brew formula repo back to before Python 2 removal for required packages

.. code-block:: sh

   cd /usr/local/Homebrew/Library/Taps/homebrew/homebrew-core
   git reset --hard 8f1d612d9e179c0d91ecd09853854e42281bd313
   cd -

6. Disable homebrew update and install ``mantid-developer`` formula (this may take a while depending on your network speed)

.. code-block:: sh

   HOMEBREW_NO_AUTO_UPDATE=1 brew install mantid-developer

7. Homebrew can stop early for reasons that are unclear. Repeat the above command until Homebrew states: *Warning: mantidproject/mantid/mantid-developer ?.? is already installed and up-to-date*.

8. Pin libraries that depend on Python 2 and libmxml and NeXus is not compatible with v3:

.. code-block:: sh

   brew pin libmxml
   brew pin sip
   brew pin pyqt

9. Unlink ``qt@4`` from ``/usr/local`` to avoid cross talk with Qt5 and ensure the webkit can be found when not linked

.. code-block:: sh

   brew unlink qt@4
   ln -s /usr/local/Homebrew/Library/Taps/mantidproject/homebrew-mantid/qt.conf /usr/local/opt/qt@4/bin/qt.conf
   ln -s /usr/local/opt/qt-webkit@2.3/include/QtWebKit /usr/local/opt/qt@4/include/QtWebKit
   ln -s /usr/local/opt/qt-webkit@2.3/lib/QtWebKit.framework /usr/local/opt/qt@4/lib/QtWebKit.framework

10. Check that ``which python`` returns a Python in ``/usr/local`` and not system Python.

11. Install python requirements

.. code-block:: sh

   python -m pip install -r /usr/local/Homebrew/Library/Taps/mantidproject/homebrew-mantid/requirements.txt


Instructions for Historic Versions
----------------------------------

See :ref:`BuildingOnOSXHistoric` for instructions for older versions of OSX.
