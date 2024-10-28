==========================================================================
 :mod:`mantid.fixtures` --- Pytest Fixtures for projects that use Mantid
==========================================================================

.. automodule:: mantid.fixtures
    :members:
    :undoc-members:
    :show-inheritance:


.. _pytest_fixtures:

Usage
-----
To make the fixtures available to your tests, add the following line, after any imports, to your conftest.py file:

.. testcode::

    pytest_plugins = "mantid.fixtures"


Once the fixtures are available, you can use them in your tests as follows:

.. testcode::

    def test_something(temp_workspace_name):
        ws = LoadEmptyInstrument(Instrument="CG3", OutputWorkspace=temp_workspace_name())
        # do stuff


or as follows:

.. testcode::

    def test_something(clean_workspace):
        ws = LoadEmptyInstrument(Instrument="CG3", OutputWorkspace='temp_ws')
        clean_workspace(ws)
        # do stuff

In both cases, when test_something( ) exits, the workspace will be deleted.

Pytest fixtures are not part of the Mantid code base, but are used by other projects that use Mantid.
Pytest fixtures are `not supported <https://docs.pytest.org/en/7.1.x/how-to/unittest.html#pytest-features-in-unittest-testcase-subclasses>`_
in test that inherit :code:`unittest.TestCase`. More information on pytest fixtures can be found
in the `pytest documentation <https://docs.pytest.org/en/7.1.x/explanation/fixtures.html#about-fixtures>`_.
