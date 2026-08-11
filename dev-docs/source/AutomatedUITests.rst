.. _AutomatedUITests:

===================
Automated UI Tests
===================

.. contents::
  :local:

Overview
########

Automated UI tests drive a real Qt interface. They build the genuine view, presenter and model,
click real widgets with ``QTest``, run real algorithms and inspect the workspaces and files that
come out. The only things mocked are the ones that would otherwise block waiting for a user: modal
error popups, confirmation prompts and generated algorithm dialogs.

They exist to replace the manual test guides in :ref:`Testing`. A guide section that a developer
used to work through by hand before a release becomes one test method, with one ``check`` per
observation the guide asked for.

The code lives in ``Testing/AutomatedUITests``:

.. code-block:: none

   Testing/AutomatedUITests/
     automated_ui_test_base.py    the harness - settings isolation, waiting, dialog patching
     qt_interaction_helpers.py    free functions for driving widgets (click, combo, table, wait)
     Example/ExampleUITest.py     a worked example, which is also the smoke test for the harness
     <Interface>/                 one directory per interface

How they relate to the other test suites
########################################

Automated UI tests are neither unit tests nor system tests, and they are deliberately kept out of
both pipelines. They are slow, they drive a GUI, and a flaky one must not be able to block a pull
request or a nightly package.

+---------------------+-------------------------+-------------------------------------------------+
| Suite               | CTest label             | When it runs                                    |
+=====================+=========================+=================================================+
| Unit tests          | ``UnitTest``            | every pull request, every nightly               |
+---------------------+-------------------------+-------------------------------------------------+
| System tests        | ``SystemTest``          | every pull request (Linux), every nightly, via  |
|                     |                         | ``runSystemTests.py`` rather than CTest         |
+---------------------+-------------------------+-------------------------------------------------+
| Automated UI tests  | ``AutomatedUITest``     | weekly, report-only                             |
+---------------------+-------------------------+-------------------------------------------------+

Three things keep them off the critical path, and a change to any one of them would put them back
on it:

* the ``AutomatedUITest`` CTest label. CI selects unit tests with ``ctest -L UnitTest``, so a test
  with any other label is invisible to it;
* the location. ``runSystemTests.py`` collects by walking ``Testing/SystemTests/tests/framework``
  and ``Testing/SystemTests/tests/qt``, so a suite under either of those would be run by the
  nightly whatever label it carried. ``Testing/AutomatedUITests`` is outside both;
* ``.github/workflows/weekly_ui_tests.yml``, which has no ``pull_request`` trigger, marks its test
  step ``continue-on-error`` and publishes the results as a check run rather than gating on them.
  It must not be added to the repository's required status checks.

macOS is not currently covered: there is no self-hosted macOS GitHub Actions runner. The workflow's
matrix has a placeholder for one.

Running them
############

They are registered with CTest, so from the build directory:

.. code-block:: sh

   # everything
   ctest -L AutomatedUITest --output-on-failure

   # one interface
   ctest -R AutomatedUITest.EngineeringDiffraction --output-on-failure

   # one module
   ctest -R AutomatedUITest.Example.ExampleUITest --output-on-failure

Most suites need data from the ExternalData store; build the ``StandardTestData`` and
``SystemTestData`` targets first, or they will report a skip. Tests run offscreen (the harness sets
``QT_QPA_PLATFORM=offscreen`` and ``MPLBACKEND=Agg`` on import), so nothing appears on screen and no
display is needed.

The data directories are named by ``PYUNITTEST_DATA_DIRS`` in
``Testing/AutomatedUITests/CMakeLists.txt``, passed to each test as ``MANTID_TEST_DATA_DIRS`` and
added to Mantid's search path in ``setUp`` - the built properties file lists only
``Testing/Data/UnitTest`` and ``Testing/Data/DocTest``, and a manual test guide typically uses the
SystemTest set. The search path is restored in ``tearDown``.

A module is an ordinary ``unittest`` file, so it can also be run directly for debugging - which is
usually easier, because you get a normal traceback:

.. code-block:: sh

   python Testing/AutomatedUITests/Example/ExampleUITest.py ExampleUITest.test_tables

Writing a test
##############

Start from ``Testing/AutomatedUITests/Example/ExampleUITest.py``. It drives a widget defined in its
own module, so it depends on no interface and no data - if it fails, the harness is broken rather
than the code under test.

The shape is:

.. code-block:: python

   from automated_ui_test_base import AutomatedUITestBase
   from qt_interaction_helpers import click, process_events, select_combo

   class MyInterfaceTest(AutomatedUITestBase):
       def setUp(self):
           super(MyInterfaceTest, self).setUp()
           self.require_files("SOMEDATA00001.nxs")   # skip cleanly if the data is not there
           self.patch_error_messages(("some.module.that.pops.a.dialog",))
           self.gui = MyInterface()
           self.gui.show()
           process_events(2)

       def tearDown(self):
           self.gui.close()
           process_events(2)
           super(MyInterfaceTest, self).tearDown()

       def test_the_guide_section_this_replaces(self):
           select_combo(self.gui.combo_instrument, "ENGINX")
           click(self.gui.button_run)

           with self.check("Guide step 4 / the output workspace is created"):
               self.assertTrue(ADS.doesExist("output"))
           with self.check("Guide step 5 / it is in d-spacing"):
               self.assertEqual(ADS.retrieve("output").getAxis(0).getUnit().unitID(), "dSpacing")

Points worth knowing before you write one:

**One ``test_`` method per scenario, not per observation.** Building an interface is expensive, and
a scenario in a manual test guide is a sequence - calibrate, then focus, then look at what was
written. Split by guide section, not by assertion.

**Use** ``self.check(label)`` **for observations.** It wraps :py:meth:`unittest.TestCase.subTest`,
so a failed observation is reported against its label and the ones after it still run. A test that
stopped at the first failure would need as many weekly runs as there are regressions. Use a plain
``self.assertX`` for preconditions - "the run number resolved", "the worker finished" - where
carrying on would only produce a cascade of meaningless failures.

**Never sleep or join.** Anything that finishes on a background thread reports back through a
queued - sometimes *blocking* queued - Qt connection, which cannot be delivered unless the calling
thread is running its event loop. Use ``wait_until`` from ``qt_interaction_helpers`` or
``self.wait_for_async_task(worker)`` from the base class; a bare ``worker.join()`` deadlocks both
threads.

**Neutralise anything modal before the first click.** An unattended test that pops a modal message
box hangs until the suite times out. ``patch_error_messages``, ``patch_confirmation_box`` and
``algorithm_dialog_runs`` on the base class cover three common cases that come up.

**Go through the interface, not around it.** Where possible use ``add_data_search_dir`` and let the
interface's own file finder resolve a run, rather than reaching past the view to inject a workspace -
what is being tested is the path a user takes.

**Do not create a ``QApplication`` at module scope.** The base class creates one lazily in ``setUp``
via ``ensure_qapp()``. Settings are isolated per test into a temporary ini file, so a test can never
read or write the real one.

**The base class must stay uncollected.** ``unittest``'s loader picks up every ``TestCase`` subclass
visible in a module, imported ones included, and falls back to a ``runTest`` method when a class has
no ``test_*`` methods. ``AutomatedUITestBase`` therefore defines neither. Keep it that way, and give
any per-interface base class you add the same treatment.

Adding a new interface
######################

Create ``Testing/AutomatedUITests/<Interface>/`` with a ``CMakeLists.txt``:

.. code-block:: cmake

   set(TEST_NAMES MyInterfaceTest.py)

   pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.MyInterface ${TEST_NAMES})

and add ``add_subdirectory(<Interface>)`` to ``Testing/AutomatedUITests/CMakeLists.txt``. The parent
directory puts the shared harness on ``PYTHONPATH``, sets the Qt API and raises the test timeout, so
nothing further is needed. Anything shared between the modules of one interface goes in a
non-test module in that directory, next to them.
