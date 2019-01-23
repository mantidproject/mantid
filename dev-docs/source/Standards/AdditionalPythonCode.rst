.. _AdditionalPythonCode:

======================
Additional Python code
======================

.. contents::
  :local:

Overview
########

Python code that is not part of 'core' Mantid, e.g. is not an algorithm, not
related to the Python exports (``mantid.*`` modules) nor an integral part of
``workbench`` should go to either the ``scripts`` directory or ``qt``
directory.

``scripts`` is a place to put non-GUI modules and scripts such as pure Python 
reduction frameworks or technique specific plotting modules which are 
desirable to be included in an official release. The code in ``scripts`` can 
be included in the automatic unit testing machinery making it possible to 
avoid breakages due to changes in Mantid's core parts. Although some reduction 
interface code lives in ``scripts``, adding more GUI related code there is 
discouraged. 

Python interfaces should go to the ``qt`` directory.

The ``scripts`` directory
#########################

Python code should be written as proper `modules 
<https://docs.python.org/3/tutorial/modules.html>`_. For example, the code of 
a module called ``spam`` should go into ``scripts/spam`` and contain a file 
named ``__init__.py`` at minimum. Then the module can be imported into Mantid 
simply by ``import spam``.

Documentation
-------------

Documentation can be added to Mantid's Python API docs in ``docs/source/api`` 
and linked to ``docs/source/api/index.rst``. Note, that it is possible to 
import Python's docstrings in ``.rst`` files using directives such as ``.. 
automodule::`` or ``.. autoclass::``, see `here 
<http://www.sphinx-doc.org/es/stable/ext/autodoc.html>`_.

Unit testing
------------

Unit tests for each module should be added to ``scripts/test/modulename``. The 
tests follow the :ref:`standard Mantid unit testing practices 
<UnitTestGoodPractice>`.

The ``qt`` directory
####################

There are no official rules for ``qt`` yet. Follow the structure of already
existing code there.
