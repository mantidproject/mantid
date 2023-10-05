.. _PipInstall:

Pip Install
===========

What is Pip?
------------

``pip`` is a package manager for Python that is primarily used for installing Python packages
from the Python Package Index (PyPI) and other package repositories. It is the default package
manager for Python and is commonly used for managing Python libraries and dependencies.

What is Conda?
--------------

``Conda`` is a package management tool that is not specific to Python. It is a cross-platform
package manager and environment management system that can handle packages written in different
programming languages, not just Python. Conda is the dependency system used in Mantid.

Can they be used together?
--------------------------

While it is possible to use ``pip`` and ``Conda`` together, there are some potential issues and
complexities when doing so:

1. **Dependency Conflicts:** When you use both ``pip`` and ``Conda`` in the same environment,
   there can be conflicts between the installed packages. These conflicts can lead to unexpected
   behavior and errors.

2. **Package Versions:** ``pip`` and ``Conda`` might have different versions of the same package,
   and managing which version takes precedence can be challenging.

3. **Environment Isolation:** Conda is designed to create isolated environments, which is
   useful for managing dependencies for different projects. Mixing ``pip`` and ``Conda`` in the
   same environment can undermine this isolation.

For these reasons, Mantid does not install any ``pip`` dependencies automatically. If you require
a PyPi package for your workflow, you can install the package manually. Note that you do so at
your own risk.

.. _pip-install-ref:

How to extend Mantid with a Pip install?
----------------------------------------

There are two ways you can ``pip`` install a package for use in Mantid. The first way is via
the command line (recommended for IDAaaS users):

.. code-block:: sh

   # Activate your environment. On IDAaaS you can activate one of the following, depending on which
   # specific major version you want to use, or the nightly:
   conda activate /opt/mantidworkbench6.7  # Or
   conda activate /opt/mantidworkbenchnightly

   # Remember to change the <insert_package_name>
   pip install <insert_package_name>

The second way is via the Python script editor window in Workbench (recommended for Windows, Linux and MacOS):

.. code-block:: python

   import subprocess, sys

   # Remember to change the <insert_package_name>
   rv = subprocess.run([sys.executable, '-m', 'pip', 'install', '--user', '<insert_package_name>'], capture_output=True)
   print(rv.stdout.decode())
   print(rv.stderr.decode())

Mantid has to be restarted for the changes to take effect. Please note that the script above requires Python 3.7 or higher.

.. categories:: Concepts
