=======================
Python Coding Standards
=======================

.. contents:: Contents
   :local:

Style
^^^^^

- Unless otherwise specified, follow `PEP 8
  <https://www.python.org/dev/peps/pep-0008>`_; this means using
  `snake_case <https://en.wikipedia.org/wiki/Snake_case>`_
- Use `YAPF <https://github.com/google/yapf>`_ to format Python code
- Always use four spaces for indentation
- For docstrings please follow `Docstring Conventions PEP 257
  <https://www.python.org/dev/peps/pep-0257>`_ and document code to
  aid `sphinx
  <https://pythonhosted.org/an_example_pypi_project/sphinx.html#full-code-example>`_

Formatting with YAPF
--------------------

To ensure that formatting matches across all developers please use YAPF version 0.28.0.
This may be installed using ``pip install --user yapf==0.28.0``.

YAPF will automatically find the configuration file when executed inside the Mantid source directory.
To format a given file you may use ``yapf -i path/to/file.py``.

To automatically format any Python files you have changed prior to commiting you may use the command: ``git diff --name-only --cached | grep '\.py' | xargs yapf -i``.
This will in place format any files with the ``.py`` extension that you have staged.

None checks
-----------  

Prefer ``if obj is not None:`` over ``if obj:``. The latter invokes
``object.__nonzero__`` whereas the former simply compares that obj
references the same object as ``None``.

License
-------

Include the following licence agreement at the top of each file, replacing ``YEAR`` with the appropriate
year:

.. code-block:: python

   # Mantid Repository : https://github.com/mantidproject/mantid
   #
   # Copyright &copy; YEAR ISIS Rutherford Appleton Laboratory UKRI,
   #     NScD Oak Ridge National Laboratory, European Spallation Source
   #     & Institut Laue - Langevin
   # SPDX - License - Identifier: GPL - 3.0 +

Imports
-------

Imports should be grouped in the following order:

1. stdlib
2. third party libraries
3. local modules

Each group should be alphabetically sorted and separate by a newline, e.g.

.. code-block:: python

    import sys

    from qtpy.QtWidgets import QMainWindow

    from mypackage.subpkg import MyClass

Python/Qt
^^^^^^^^^

Use the `qtpy <https://pypi.python.org/pypi/QtPy>`_ module to hide the
differences between PyQt versions.  When working with signals, use the
new style. For naming new custom signals, use the ``sig_`` prefix:

.. code-block:: python

    from qtpy.QtCore import Signal
    ...

    class MyWidget(...):
        """Funky new widget"""    

        # Signals
        sig_run_a_thing_happened = Signal(str, str, str, bool, bool)
