.. _Libraries:

=========
Libraries
=========

.. contents:: Contents
   :local:

Summary
^^^^^^^

This page describes the preferred libraries used with the Mantid code
base.

General
^^^^^^^

- Use libraries from `std <http://en.cppreference.com/w/cpp>`_ where possible.
- `Boost <http://www.boost.org>`_ header-only libraries are also
  always available. The following compiled Boost libraries are
  available:

  - regex
  - date_time
  - python
  - serialization
  - others required must be discussed with the TSC

- Eigen is used for fast linear algebra calculations
- Poco is also used for:

  - asynchronous method support
  - application configuration
  - logging
  - path handling (will eventually be replaced by std::filesystem)
  - networking
  - XML parsing

Specific Recommendations
^^^^^^^^^^^^^^^^^^^^^^^^

Regex
-----

Prefer ``boost::regex`` over ``std::regex`` (bugs in ``std::regex``
until gcc 5 and some platforms still use gcc 4.8)

String Algorithms (chop, ends_with etc)
---------------------------------------

- For any string functions that are not implemented in the C++ standard, prefer Boost algorithms if possible
- Some string utilities also exist in ``MantidKernel/Strings.h``
