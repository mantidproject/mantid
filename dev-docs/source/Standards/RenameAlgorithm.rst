.. _RenameAlgorithm:

=====================
Renaming an Algorithm
=====================

.. contents::
  :local:

Sometime developers will run into situations where the capability of the algorithm grows
beyond its original designated name, therefore a renaming of the algorithm is necessary
to ensure the name of the algorithm faithfully represent the functionality of given algorithm.
This document provides a recommended process to rename the algorithm while avoiding introducing
breaking changes for the general users.


Algorithm Naming Convention
###########################

As a general purpose data process platform for global users, the names of the Mantid algorithms need
to be as self evident as possible so that users can intuitively understand the functionality and limitations
of the algorithms they are using.
To this end, it is important for Mantid developers to use clear and concise names during the renaming process.
Generally speaking, the name of a algorithm can contain up to **four** sections:

.. admonition:: Mantid Algorithm Naming Convention

  [Technique] [Instrument/Facility] Action Target

* Technique

  If given algorithm is designed for a specific technique, the algorithm name should start with the abbreviation of the
  technique.
  However, this section can be omitted if the algorithm is not technique specific (such as file loading and data plotting).
  Here are some commonly used abbreviations of techniques

  =============  ===========================================
  Abbreviations           Full Description
  =============  ===========================================
  **CWPD**           Constant Wavelength Powder Diffraction
  **PD**             Powder Diffraction
  **REFL**           Reflectometry
  **SANS**           Small Angle Neutron Scattering
  **SCD**            Single Crystal Diffraction
  =============  ===========================================

  .. note::

    The table above is a work in progress as more abbreviations will be added in the future.

* Instrument/Facility

  As Mantid is a collaboration across many different institutes, it is very common to have some algorithms that are specifically
  designed for a special instrument or a facility.
  For algorithms like these, it is important to have the abbreviations of the corresponding instrument or facility clearly shown
  in the name.
  On the other hand, this section can be skipped if the algorithm is general enough that its application is no longer tied to a
  specific instrument or facility.

  * Here are some commonly used abbreviations of facilities

  =============  ===========================================
  Abbreviations           Full Description
  =============  ===========================================
  **ILL**           Institut Laue-Langevin at GRENOBLE,France
  **ISIS**          ​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​​ISIS Neutron and Muon Source at UK
  **HFIR**          High Flux Isotope Reactor at ORNL,USA
  **SNS**           Spallation Neutron Source at ORNL,USA
  =============  ===========================================

  .. note::

    The table above is a work in progress as more abbreviations will be added in the future.

  * Here are some commonly used abbreviations of instruments

  =============  ====================================================
  Abbreviations           Full Description
  =============  ====================================================
  **CORELLI**    Elastic Diffuse Scattering Spectrometer at BL-9, SNS
  **POWGEN**     Powder Diffractometer at BL-11A, SNS
  **TOPAZ**      Single-Crystal Diffractometer at BL-12, SNS
  **WAND2**      Wide-Angle Neutron Diffractometer at HB-2C, HFIR
  =============  ====================================================

  .. note::

    The table above is a work in progress as more abbreviations will be added in the future.

* Action

  As data process platform, Mantid perform various action via algorithms, therefore it is crucial to have clear and concise description
  of intended action depicted in the algorithm name.

* Target

  Most of the time the action term above requires a specific receiving end, namely a target.
  Depending on the action, sometimes the target can be omitted if it is self evident (such as ``LoadFiles`` can be simplified into ``Load``).

.. admonition:: Example

  ``SCDCalibratePanels`` indicates this is a algorithm designed for single crystal diffraction technique that is not
  tied to a specific instrument or facility.
  It performs calibration of panel type detectors.

Rename C++ Algorithm
####################

Renaming a C++ algorithm can be achieved via the following steps:

* Do a grep search (or use Github search) to locate files that call this algorithms.
* Rename the algorithm (header, source and unit test)

  * Rename the header and set the original name as alias

  .. code-block:: c++

    #include "MantidAPI/DeprecatedAlias.h"
    ...
    class DLLExport NewAlgName : public API::Algorithm, public API::DeprecatedAlias {
        ...
        const std::string alias() const override { return "OriginalAlgName"; };
        ...
    }

  * Set the deprecation date (the date this algorithm name changed) in the constructor in source file

  .. code-block:: c++

    //-----------------------------------------------------------------------------------
    /** Constructor
    */
    NewAlgName::NewAlgName(){
        setDeprecationDate("2021-09-14"); // date string formatted like the example here
    }

  * Update tests

    Unit test and system tests should be the place to start with the renaming update.

  * Update documentation page and corresponding examples

* Update calls within Mantid to use the new Algorithm name

* Make sure list the name change in the release notes

* [Optional] Inform the users about the name change once pull request is merged

  .. note::

    Script ``buildconfig/move_class.py`` can help facilitate the file renaming process
    of an existing c++ algorithm. More specifically, this script will rename the header,
    source and unit test files, as well as taking care of the renaming within the
    corresponding CMake file. Still, the developer needs to dive into the source files
    to search and replace the class name manually.

    Assumming our algorithm ``OldAlgName`` resides in the ``DataHandling`` namespace,
    we would write:

  .. code-block:: bash

    python2.7 move_class.py DataHandling OldAlgName DataHandling NewAlgName


Rename Python Algorithm
#######################

Goal: given existing algorithm `AlgOldName`, we want to rename it to `AlgNewName`, and `AlgOldName` will
become a deprecated alias of `AlgNewName`.

* Replace

Replace all occurrences of `AlgOldName` with `AlgNewName` in all files. In Linux or Mac:

  .. code-block:: bash

    grep -rl AlgOldName . | xargs sed -i 's/AlgOldName/AlgNewName/g'

The names of some files will need to be replaced. Typically these will be the algorithm file, test file, and documentation

  - `algOldName.py`  (becomes "algNewName.py")
  - `algOldNameTest.py` (becomes "algNewNameTest.py")
  - `algOldName-v1.rst` (becomes "algNewName-v1.rst")

* Edit

Edit `algNewName.py`. We need to add and alias method and mark the algorithm with the alias deprecator

Below are the relevant statements to deprecate the alias on Christmas day of the year 2025

  .. code-block:: python

    from mantid.utils.deprecator import alias_deprecated

    @deprecated_alias('2025-12-25')
    class AlgNewName(PythonAlgorithm):

        def alias(self):
            r"""Alternative name to this algorithm"""
            return 'algOldName'


Configuration
=============

Upon using a deprecated alias to invoke an algorithm, a message will be printed in the log at the `error`
level. For instance, when using deprecated alias `algOldName` in place of the algorithm's name `algNewName`,
the following error message is printed:

  .. code-block:: bash

    Algorithm alias algOldName is deprecated. Use algNewName instead

If so desired, the user can raise a ``RuntimeError`` by setting property ``algorithms.alias.deprecated`` to
``Raise`` in the user properties file `$HOME/.mantid/Mantid.user.properties` or in a script:

  .. code-block:: python

    from mantid.kernel import ConfigService
    config = ConfigService.Instance()
    config['algorithms.alias.deprecated'] = 'Raise'

Coming to our previous example, a ``RuntimeError`` is printed:

  .. code-block:: bash

    RuntimeError: Use of algorithm alias algOldName not allowed. Use algNewName instead
    File "/home/username/my_script.py", line 9, in <module>
        def alias(self):
    File "/path/to/mantid/Framework/PythonInterface/mantid/simpleapi.py", line 1032, in __call__
        raise RuntimeError(f'Use of algorithm alias {self._alias.name} not allowed. Use {name} instead')

To prevent the ``RuntimeError`` and instead print a log error message, the property can be left unset or set to
"``Log``"