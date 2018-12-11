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
interface code lives ``scripts``, adding more GUI related code there is 
discouraged. 

New Python interfaces should go to the ``qt`` directory.

The ``scripts`` directory
#########################

New Python code
---------------

New code should go in ``scripts/modulename`` directory. They can then be
imported into Mantid simply by ``import modulename``.

Documentation
-------------

Currently, there is no official place to put documentation for the code in
``scripts``. However, *Concepts* and *Techniques* categories should cover most
use cases of ``scripts``.

Unit testing
------------

Unit tests should be added to ``scripts/test/modulename``. The tests follow the
:ref:`standard Mantid unit testing practices <UnitTestGoodPractice>`.

The ``qt`` directory
####################

There are no official rules for ``qt`` yet. Follow the structure of already
existing code there.
