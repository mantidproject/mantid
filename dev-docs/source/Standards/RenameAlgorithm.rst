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

    python move_class.py DataHandling OldAlgName DataHandling NewAlgName


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
"``Log``".
