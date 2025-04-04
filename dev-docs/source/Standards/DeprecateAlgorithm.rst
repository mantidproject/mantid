.. _DeprecateAlgorithm:

========================
Deprecating an Algorithm
========================

.. contents::
  :local:

When the lifetime of an algorithm reaches its limit, it is good practice to signal developers and users of this
fact. This signal gives them time to make arrangements for their scripts, either to stop using the algorithm or
to use another algorithm. The signal is implemented via "deprecation".

For the current list of deprecated algorithms, visit the
`Deprecated category page <https://docs.mantidproject.org/nightly/algorithms/categories/Deprecated.html>`_.

Deprecating a C++ Algorithm
===========================

To deprecate an existing C++ algorithm, its associated class must:

* Inherit from class `DeprecatedAlgorithm <https://doxygen.mantidproject.org/nightly/d5/d26/classMantid_1_1API_1_1DeprecatedAlgorithm.html>`_.
* Invoke method ``DeprecatedAlgorithm::deprecatedDate(const std::string &)`` in the algorithm's constructor.
* If another algorithm should be used in place of the deprecated one, invoke method ``void DeprecatedAlgorithm::useAlgorithm(const std::string &, const int version = -1);`` in the algorithm's constructor.

Here are the relevant lines for deprecating algorithm ``MyOldAlg`` on Christmas day in 2020 and informing that
``MyNewAlg`` should be used

.. code-block:: c++

  #include "MantidAPI/DeprecatedAlgorithm.h"

  class MANTID_ALGORITHMS_DLL MyOldAlg : public API::Algorithm, public API::DeprecatedAlgorithm {
  public:
  MyOldAlg::MyOldAlg() : m_someMember(0) {
    useAlgorithm("MyNewAlg");
    deprecatedDate("2020-12-25");
  }


Deprecating a Python Algorithm
==============================

To deprecate a Python algorithm, we decorate its associated class with
``mantid.utils.deprecator.deprecated_algorithm``. In the example below we deprecated ``MyOldAlg`` on
Christmas day in 2020, and inform that ``MyNewAlg`` should be used in place of ``MyOldAlg``.

.. code-block:: python

  from mantid.utils.deprecator import deprecated_algorithm

  @deprecated_algorithm('MyNewAlg', '2020-12-25')
  class MyOldAlg(PythonAlgorithm):
      pass # continued with class definition

If there is no other algorithm to be used in place of ``MyOlAlg``, then an empty string or ``None`` must be
passed, e.g ``@deprecated_algorithm(None, '2020-12-25')``


Configuration
=============

Upon using a deprecated algorithm, a message will be printed in the logs at the `error`
level. For instance, when using algorithm `MyOldAlg` (deprecated on Christmas day in 2020) in place of
the new algorithm `MyNewAlg`, the following error message is printed:

.. code-block:: bash

  Algorithm "MyOldAlg" is deprecated since 2020-12-25. Use "MyNewAlg" instead

If so desired, the user can raise a ``RuntimeError`` by setting property ``algorithms.deprecated`` to
``Raise`` in the user properties file `$HOME/.mantid/Mantid.user.properties`, or in a script using `ConfigService`:

.. code-block:: python

  from mantid.kernel import ConfigService
  config = ConfigService.Instance()
  config['algorithms.deprecated'] = 'Raise'

Coming to our previous example, an error message is printed

.. code-block:: bash

  Error in execution of algorithm MyOldAlg
  Configuration "algorithms.deprecated" set to raise upon use of any deprecated algorithm


To prevent the previous error and instead print a log error message, `algorithms.deprecated` can be
left unset or set to "``Log``".

.. note::

  Debug builds set ``algorithms.deprecated`` to ``Raise``, the purpose is twofold. First, prevent developers
  from introducing new source invoking deprecated algorithms. Second, encourage developers to refactor existing
  source that invokes deprecated algorithms.
