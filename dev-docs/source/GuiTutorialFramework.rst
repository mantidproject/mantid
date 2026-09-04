.. _GuiTutorialFramework:

======================
GUI Tutorial Framework
======================

.. contents::
  :local:

Overview
########

``mantidqt.widgets.tutorial`` runs a guided walkthrough of a Qt interface: it dims the window,
spotlights one widget at a time, explains what it does, and **performs the interaction itself** so
the user watches a real workflow run rather than reading about one.

**Explain first, act second.** A step opens whatever it points at, spotlights it and shows its
caption, and then waits. The action runs when the user presses *Show me*, so they are reading the
explanation of a control at the moment they watch it being used.

*Next* performs the step's action first if *Show me* was not pressed,
since chapters are cumulative and a skipped action would leave every later step describing an
interface that never reached the state it assumes.

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

Consequences of this:

* The tour can perform genuinely destructive-looking actions — loading files, running algorithms,
  writing exports — because none of it touches anything the user owns.
* Jumping to a chapter is a *rebuild*, not an undo. There is no need for a step to know how to
  reverse itself.
* Your interface must be safe to instantiate twice. In practice that means workspace names must be
  unique per instance, and closing the window must clean up after itself.

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

Two traps of this pattern:

* **Usage reporting.** If the view registers a feature usage in its constructor, the sandbox will
  double-count every real launch. Give the view a flag to skip it.
* **Recursion.** Any method the presenter has of starting the tutorial, the sandbox's presenter also has.
  Give the presenter a flag to suppress it, these methods so tutorials can't be launched by tutorials.

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
            ),
        ],
    )

``target`` and ``action`` are **callables taking the sandbox**, never captured widgets. There are
two good reasons for this: some widgets do not exist for access when the tour is written (a matplotlib
canvas injected into a placeholder, a table cell widget that appears once there are rows), and a
chapter is replayed against a freshly built interface whenever the user jumps to it.

``await_`` is polled after the action until it holds — this is how a step waits for slow work
without blocking, and the step is not reported as done until it does. Give it an explicit
``await_timeout_s``: how long is reasonable depends entirely on what is being waited for.

A step's target is opened up *before* its action runs — the containing tab selected, a collapsed
group box expanded, including the target itself when it is one. A value set inside a shut section
changes nothing the user can see. A target that does not exist until the action has run is
allowed: it is simply not highlighted while the step is being explained, and is only reported as
missing if it is still absent afterwards.

Where the controls live
#######################

Two main interface components:

* ``TutorialShell`` wraps the interface. Chapter tabs along the top, ``Back`` / ``Show me`` /
  ``Next`` / ``End tutorial`` and a step counter along the bottom, with the interface reparented in
  between. It does not move. ``Show me`` is hidden for a step that only explains something, and
  disabled once the step has been performed.
* ``TutorialBubble`` is the caption beside the highlight, and carries **no controls at all**. It
  chases whatever is being spotlighted, so a button on it would be somewhere different on every
  step.

Because the interface becomes a child of the shell, the dimming overlay covers only the interface:
the tabs and buttons stay bright and usable while everything they act on is dimmed.

**Pointing into another window.** An interface is rarely one window, and a tour that explains a
settings dialog has to annotate something the interface's own overlay cannot reach into.
``TutorialAnnotator`` handles that: it owns an overlay and a caption *per window*, builds them as
they are needed, and shows only the pair belonging to whatever is currently spotlighted. The player
is given the annotator in place of an overlay and a bubble and never learns how many windows are
involved.

One rule inside it is worth knowing before changing it: anything inside the primary widget is
annotated on the **primary itself**, not on its window. The interface is a child of the shell, so
dimming its window would put the dimmer over the chapter tabs and the navigation - the controls the
user needs to drive the tour. Everything else is annotated on its own window, which is what reaches
a dialog.

No shipped tour points into a dialog yet. Two things to check before one does. The dialog must be
made **non-modal**, in the sandbox and never in the interface itself, or it blocks the tutorial's
own controls until the user closes something they did not ask for. And mind what applying it
writes: a settings dialog that saves to Mantid's shared ``QSettings`` is *not* isolated by the
sandbox, so such a tour must dismiss it rather than accept it - Ok or Apply would change the real
interface's saved settings on the user's behalf.

Give a step an action only when it *does* something the user can watch. ``Show me`` is offered
whenever a step has one, so an action whose effect is invisible - re-expanding a section the reveal
has already opened, say - puts a button on screen that appears to do nothing when pressed.

Where a step's effect shows up somewhere other than where the annotation is pointing, name that somewhere
in ``avoid``. The caption is placed clear of it as well as of the spotlight, so a step that ticks a
check box while the values it changes are displayed elsewhere does not end up explaining a change that
is blocked from view:

.. code-block:: python

    TutorialStep(
        title="Seeing them in the lab frame",
        text="The fields go read-only, because these values are derived.",
        target=lambda sandbox: sandbox.view.chkLabDirs,
        avoid=lambda sandbox: sandbox.view.groupBox_textureVectors,
        action=lambda sandbox: set_check_state(sandbox.view.chkLabDirs, True),
    )

``avoid`` takes one widget or a sequence of them, and a miss is ignored - keeping out of the way is
a courtesy, not something worth interrupting the tour over.

Navigation is refused while a step is settling or working (``busy_changed``). Without that, Next
pressed mid-calculation would run the following step's action against an interface that had not
finished reacting to this one - and Next pressed during the settle would skip the step's caption
while its action had already run.

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
renamed — silently, because nothing else imports it. Write a test that plays the tour against a real
interface, pressing Next as each step becomes ready and cutting ``settle_ms`` to the minimum, and
assert that no step failed and that every step found what it points at. See
``mantidqtinterfaces/TexturePlanner/tutorial/test/test_chapters.py``.

Put each step's observation in its own ``subTest`` so one broken step does not hide the rest, and
assert that the sandbox leaves no workspaces behind once it is closed. Playing the tour runs the
interface for real, so play it as few times as its distinct paths require - once through, and once
per chapter entered by itself, since a chapter jump reaches its starting state by fast-forwarding
rather than by walking - and assert everything else against what that recorded. Build the hurried
copy of the tour with ``dataclasses.replace`` rather than by listing the fields, or the test will
quietly stop matching the real steps the next time ``TutorialStep`` gains one.
