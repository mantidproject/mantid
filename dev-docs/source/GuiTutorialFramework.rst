.. _GuiTutorialFramework:

======================
GUI Tutorial Framework
======================

.. contents::
  :local:

Overview
########

``mantidqt.widgets.tutorial`` runs a guided, automated walkthrough of a Qt interface: it dims the
window, spotlights one widget at a time, explains what it does, and **performs the interaction
itself** so the user watches a real workflow run rather than reading about one.

An interface supplies two things — a factory that builds a copy of itself, and a list of steps.
Everything else is handled by the framework.

The first user of it is the :ref:`Texture Planner <Texture_Planner-ref>`
(``mantidqtinterfaces/TexturePlanner/tutorial/``), which is the working example to read alongside
this page.

The sandbox: why the tour never drives the user's window
########################################################

A tour that typed into the interface the user had open would be unusable — it would overwrite a
half-finished setup, and could not be offered on startup without risking someone's work. So the
tour builds and drives **a second, throwaway instance** of the interface, and closes it at the end.

This is what makes the rest of the design possible, and it has consequences worth knowing:

* The tour can perform genuinely destructive-looking actions — loading files, running algorithms,
  writing exports — because none of it touches anything the user owns.
* Jumping to a chapter is a *rebuild*, not an undo. There is no need for a step to know how to
  reverse itself.
* Your interface must be safe to instantiate twice. In practice that means workspace names must be
  unique per instance, and closing the window must clean up after itself. The Texture Planner
  already did both.

A sandbox is any object with a ``window`` attribute and a ``teardown()`` method. It is also passed
to every step as the *context*, so put on it whatever the steps need to reach:

.. code-block:: python

    class TutorialSandbox:
        def __init__(self, parent=None):
            self.data = DemoData()                       # generated demo files
            self.model = MyModel()
            self.view = MyView(parent=parent, register_usage=False)
            self.presenter = MyPresenter(self.model, self.view, offer_tutorial=False)

        @property
        def window(self):
            return self.view

        def teardown(self):
            self.view.close()                            # cleans up via closeEvent
            self.view.deleteLater()
            self.data.cleanup()

Two traps that both instances of this pattern have hit:

* **Usage reporting.** If the view registers a feature usage in its constructor, the sandbox will
  double-count every real launch. Give the view a flag to skip it.
* **Recursion.** If the presenter offers a tutorial, the sandbox's presenter will offer one too —
  and on a first-ever open, launch it. Give the presenter a flag to suppress it.

Writing steps
#############

A tour is a sequence of ``TutorialChapter``, each a sequence of ``TutorialStep``:

.. code-block:: python

    from mantidqt.widgets.tutorial.interaction import click, select_combo
    from mantidqt.widgets.tutorial.step import TutorialChapter, TutorialStep

    SETUP = TutorialChapter(
        name="Sample setup",
        description="Load a sample and give it a material.",
        steps=[
            TutorialStep(
                title="Load a sample shape",
                text="The shape comes from an <b>STL mesh</b> or a <b>CSG description</b>.",
                target=lambda sandbox: sandbox.view.finder_xml,
                action=lambda sandbox: set_finder(sandbox.view.finder_xml, sandbox.data.path),
                await_=lambda sandbox: not sandbox.view.finder_xml.isSearching(),
                await_timeout_s=60.0,
                await_text="Looking for the sample file…",
                dwell_ms=4500,
            ),
        ],
    )

``target`` and ``action`` are **callables taking the sandbox**, never captured widgets. Two reasons,
and both bite: the interesting widgets often do not exist when the tour is written (a matplotlib
canvas injected into a placeholder, a table cell widget that appears once there are rows), and a
chapter is replayed against a freshly built interface whenever the user jumps to it.

``await_`` is polled until it holds before the step is narrated — this is how a step waits for slow
work without blocking. Give it an explicit ``await_timeout_s``: how long is reasonable depends
entirely on what is being waited for, and a default would only ever be wrong in the direction that
hides a hang.

Never block
###########

Everything the tour shows — the spotlight, the caption, the interface reacting — is painted by the
event loop, so a tour that waits by blocking freezes the demonstration it is giving. Two rules
follow:

* Wait with ``interaction.wait_for(predicate, on_ready, timeout_s)``, which polls on a ``QTimer``
  and calls back. Never ``time.sleep``, ``QTest.qWait``, or a spin loop around ``processEvents``.
* Actions must **finish before they return**. ``interaction.click`` deliberately does not use
  ``animateClick`` for this reason: that releases the button from a timer ~100 ms later, so the
  press can still be pending after the tour has moved on — or after the tour has ended and the
  interface it belonged to has been torn down, at which point the handler runs against workspaces
  that no longer exist.

Driving widgets
###############

``mantidqt.widgets.tutorial.interaction`` holds the helpers, which exist because the naive Qt call
usually does not do what you want:

.. list-table::
  :header-rows: 1

  * - Helper
    - Why not the obvious call
  * - ``set_text``
    - ``setText`` emits none of ``textEdited`` / ``editingFinished`` / ``returnPressed``, so the
      interface never notices the value.
  * - ``select_combo``
    - ``setCurrentText`` on a non-editable combo silently does nothing when the text is absent.
  * - ``select_tab``
    - By title, not index — indices move when tabs are added conditionally.
  * - ``set_spin_box``
    - Spin boxes clamp and round silently.
  * - ``set_check_state``
    - Synchronous, unlike ``click``, because the caller asked for a *state*.
  * - ``ensure_visible``
    - Selects the containing tab, expands a collapsed group box, and scrolls the widget into view.
      A target that is not on screen has a geometry that is meaningless to point at.

These overlap with ``Testing/AutomatedUITests/qt_interaction_helpers.py``, which solves the same
problems for tests. They are deliberately *not* shared: the test helpers raise ``AssertionError``
(right for a test, wrong for production) and wait by blocking (right for a test, fatal here).

Launching it
############

.. code-block:: python

    from mantidqt.widgets.tutorial.launcher import run_tutorial, should_show_on_startup

    def open_tutorial(self, mark_as_seen=False):
        self._tutorial_session = run_tutorial(
            sandbox_factory=make_sandbox_factory(parent=self.view),
            chapters=CHAPTERS,
            parent=self.view,
            settings_key="MyInterface",
            mark_as_seen=mark_as_seen,
        )

Keep the returned session alive for as long as the tour runs — it owns the sandbox window.

``should_show_on_startup(key)`` / ``mark_seen(key)`` store a flag under
``CustomInterfaces/<key>/tutorial_seen``. The flag is set when the tutorial is *offered*, not when
it is completed: someone who closes it immediately has made their decision, and the toolbar button
keeps it available. Offer it from the event loop rather than from the presenter's constructor, or
the window it should appear over has not been laid out yet:

.. code-block:: python

    if should_show_on_startup("MyInterface"):
        QTimer.singleShot(0, lambda: self.open_tutorial(mark_as_seen=True))

Testing a tour
##############

A tour is written against widget names and presenter methods, so it drifts the moment either is
renamed — silently, because nothing else imports it. Write a test that plays every chapter against
a real interface, overriding the delays so it runs at full speed, and assert that no step failed
and that every step found what it points at. See
``mantidqtinterfaces/TexturePlanner/tutorial/test/test_chapters.py``.

Put each step's observation in its own ``subTest`` so one broken step does not hide the rest, and
assert that the sandbox leaves no workspaces behind once it is closed.
