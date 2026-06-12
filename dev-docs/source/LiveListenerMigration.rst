.. _LiveListenerMigration:

Live-Listener API Migration Guide (v3 refactoring)
==================================================

Mantid's live-listener interface was refactored to separate state-read
from state-transition, eliminate hidden side effects in ``runStatus()``,
and allow :ref:`LoadLiveData <algm-LoadLiveData>` to run successfully as
a stand-alone algorithm without requiring
:ref:`MonitorLiveData <algm-MonitorLiveData>`.

This document covers the generic interface changes that apply to every
``ILiveListener`` subclass.  For details specific to
``SNSLiveEventDataListener`` (ADARA packet handling, the deferred
``RunStatusPkt`` flow, the state-machine diagrams, and the pre-existing
``m_ignorePackets`` defect) see :ref:`SNSLiveEventDataListenerRefactoring`.


.. contents::
   :local:
   :depth: 2


Motivation
----------

The pre-refactor interface had three coupled defects:

1. **Conflated state.**  ``runStatus()`` returned a value that was part
   DAS state (``NoRun / BeginRun / Running / EndRun``) and part
   listener-internal FSM — it mutated to ``Running`` or ``NoRun`` as a
   side effect of being called.  Callers could not ask either question
   without paying the cost of the other.

2. **Hidden side effects on a "getter".**  Calling ``runStatus()``
   cleared the geometry cache, the name map, marked the workspace
   uninitialised, ran ``initWorkspacePart1()``, consumed any deferred
   ``RunStatusPkt``, and dropped the back-pressure flag
   ``m_pauseNetRead``.  None of this was visible at the call site.

3. **External-control deadlock.**  Stand-alone ``LoadLiveData`` does not
   call ``runStatus()``.  After a ``BeginRun`` / ``EndRun`` packet set
   ``m_pauseNetRead = true``, the background reader stopped, the
   workspace could not be re-initialised, and the next
   ``extractData()`` either hit the 10-second ``NotYet`` timeout or
   spun indefinitely on a stale workspace.

A fourth issue was an undocumented ordering contract: the legacy
``MonitorLiveData`` polling loop relied on calling ``extractData()``
first and ``runStatus()`` second within the same iteration; nothing in
the type system enforced this.

The v3 refactor has three objectives, in priority order:

1. Expose the listener state and the ADARA run state without conflating
   them.
2. Separate state-read from state-transition; make every state mutation
   explicit and named.
3. Move as much of the state machine as possible into ``extractData()``,
   the only method every consumer of ``ILiveListener`` must call.

The result is four pure-getter queries on ``ILiveListener``, a
template-method ``extractData()`` on ``LiveListener`` that dispatches to
named protected hooks, and a deprecated-but-functional ``runStatus()``
shim for backward compatibility.


New API (use these)
-------------------

All four methods are **pure getters** — calling them never mutates
internal state.  They are declared on ``ILiveListener``.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Method
     - Meaning
   * - ``runState() const``
     - Current DAS run state: ``NoRun``, ``BeginRun``, ``Running``, or
       ``EndRun``.  Updated by the background thread; readable from the
       foreground at any time.  Default implementation returns
       ``NoRun`` for listeners with no concept of run boundaries.
   * - ``isPaused() const``
     - ``true`` when the DAS has signalled a run pause (ADARA
       annotation packet).  Orthogonal to ``runState()`` — the run
       state remains ``Running`` while paused.  Default returns
       ``false``.
   * - ``listenerState() const``
     - Connection / health: ``Disconnected``, ``ReadWait``,
       ``Connected``, or ``Error``.  **Pure virtual** — every concrete
       listener must override it.
   * - ``lastTransition() const``
     - ``std::optional<RunStatus>`` — the edge (``BeginRun`` or
       ``EndRun``) committed by the most recent ``extractData()`` call,
       or ``std::nullopt`` if none.  Cleared automatically on the
       *next* successful ``extractData()`` cycle that commits a *new*
       edge, so that ``MonitorLiveData`` does not re-process the same
       edge.  Default returns ``std::nullopt``.

The base-class declarations are:

.. code-block:: cpp

   namespace Mantid::API {

   enum class ListenerState {
       Disconnected,   ///< Not connected
       Connected,      ///< Connected and reading
       ReadWait,       ///< Connected but paused at a run boundary
       Error           ///< Background thread reported an exception
   };

   class MANTID_API_DLL ILiveListener : public Kernel::PropertyManager {
   public:
       enum RunStatus { NoRun = 0, BeginRun = 1, Running = 2, EndRun = 4 };

       virtual RunStatus runState() const                  { return NoRun;       }
       virtual bool      isPaused() const                  { return false;       }
       virtual ListenerState listenerState() const = 0;
       virtual std::optional<RunStatus> lastTransition() const { return std::nullopt; }

       virtual std::shared_ptr<Workspace> extractData() = 0;

       [[deprecated("Use runState() / lastTransition() and call extractData() "
                    "to commit pending transitions.")]]
       virtual RunStatus runStatus();
       // ... other existing methods unchanged ...
   };

   } // namespace Mantid::API

Two questions are orthogonal and answered by independent getters:

* ``runState()`` answers "what is the DAS doing?"
* ``isPaused()`` answers "is the current run paused?"

A caller inspecting ``runState()`` sees ``Running`` whether the run is
paused or not; a caller inspecting ``isPaused()`` sees the pause flag
regardless of run phase.  Neither query reads or mutates the other's
backing field.


Deprecated API (migrate away from)
-----------------------------------

``runStatus()``
~~~~~~~~~~~~~~~

The old ``runStatus()`` method carried large hidden side effects:

* Cleared geometry and name-map caches.
* Re-initialised the workspace.
* Consumed the deferred ``RunStatusPkt`` (SNS only).
* Cleared the ``m_pauseNetRead`` gate (causing deadlock in stand-alone
  ``LoadLiveData``).

It is replaced by the combination of ``lastTransition()`` and
``runState()``.  ``runStatus()`` is no longer pure virtual; the
default base-class implementation in ``ILiveListener.cpp`` is:

.. code-block:: cpp

   RunStatus ILiveListener::runStatus() {
       if (auto edge = lastTransition()) return *edge;
       return runState();
   }

This shim preserves backward-compatible behaviour for
``MonitorLiveData`` without requiring callers to change immediately.
The method is marked ``[[deprecated]]`` and **will be removed in a
future release**.  Note that the default cannot throw on its own: both
``lastTransition()`` and ``runState()`` have safe defaults
(``nullopt`` / ``NoRun``).  A listener that wishes to forbid the
deprecated API outright is free to override ``runStatus()`` as
``[[noreturn]] throw std::logic_error(...)``; nothing in the design
relies on the default never throwing.

How the shim preserves ``MonitorLiveData`` semantics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``MonitorLiveData`` calls ``extractData()`` first (via
``loadAlg->executeAsChildAlg()``) and ``runStatus()`` second within the
same loop iteration.  By the time ``runStatus()`` is called,
``extractData()`` has already committed any pending transition and
``lastTransition()`` is populated.  The shim returns the edge once
across the boundary, then falls through to ``runState()`` on
subsequent calls (``Running`` or ``NoRun``) — matching the historical
return-once-then-settle behaviour exactly.


``extractData()`` as a template method
---------------------------------------

The base class ``API::LiveListener`` (from which every in-tree concrete
listener derives) **finalises** ``extractData()`` and dispatches to three
protected hooks:

.. code-block:: cpp

   // In Framework/API/inc/MantidAPI/LiveListener.h
   class MANTID_API_DLL LiveListener : public ILiveListener {
   public:
       std::shared_ptr<Workspace> extractData() final;   // template method

   protected:
       /// Foreground-thread hook called at the very start of extractData(),
       /// before any workspace construction.  Default no-op.
       virtual void onBeforeExtract() {}

       /// The actual workspace construction step.  Pure virtual.
       virtual std::shared_ptr<Workspace> doExtractData() = 0;

       /// Success-only hook called after doExtractData() returns normally,
       /// before the workspace is returned to the caller.  Default no-op.
       /// Not called if doExtractData() throws.
       virtual void onAfterExtract() {}
   };

The template-method body is:

.. code-block:: cpp

   std::shared_ptr<Workspace> LiveListener::extractData() {
       onBeforeExtract();          // Phase 1: pre-extract bookkeeping
       auto ws = doExtractData();  // Phase 2: build the workspace
       onAfterExtract();           // Phase 3: success-only post-extract work
       return ws;
   }

This three-phase split has the following consequences for listener authors:

1. **Override ``doExtractData()``, not ``extractData()``.**
   ``extractData()`` is ``final``; the compiler will reject any
   override.  Move your existing ``extractData()`` body into
   ``doExtractData()`` unchanged.

2. **Use ``onBeforeExtract()`` for pre-extract work; use
   ``onAfterExtract()`` for success-only post-extract work.**
   ``onBeforeExtract()`` runs unconditionally at the start of every
   ``extractData()`` call — this is where the SNS listener commits
   queued ``BeginRun`` transitions (see
   :ref:`SNSLiveEventDataListenerRefactoring`).
   ``onAfterExtract()`` runs only when ``doExtractData()`` returned
   normally — this is where the SNS listener dispatches ``EndRun``
   and clears the delivered transition edge.

3. **Exception safety across the three-phase split.**  If
   ``doExtractData()`` throws (e.g. ``Exception::NotYet``),
   ``onAfterExtract()`` is **not** called.  The side effects of
   ``onBeforeExtract()`` — including any ``m_lastTransition``
   assignment — are preserved across the retry.  Edge-clearing must
   not run on a retry path; the SNS listener achieves this with a
   ``m_previousExtractCompleted`` flag that gates the deferred clear
   in the next ``onBeforeExtract()`` — the flag is only set at the
   end of ``onAfterExtract()``, so a ``NotYet`` from any phase leaves
   it ``false`` and the edge survives intact.  See the
   :ref:`C1 invariant <migration-c1-invariant>` for details.


State-transition hooks
----------------------

Listeners that need to react to run boundaries should override the
*hook* methods rather than the getters:

.. list-table::
   :header-rows: 1
   :widths: 22 18 60

   * - Hook
     - Thread
     - When called
   * - ``onBeginRun()``
     - foreground
     - Called from ``onBeforeExtract()`` when a ``BeginRun``
       transition is pending.  The new run details are available at
       this point.
   * - ``onEndRun()``
     - foreground
     - Called from ``onAfterExtract()`` when an ``EndRun``
       transition is pending.  Deferred to after ``doExtractData()``
       so the finishing run's accumulated events are harvested before
       the workspace buffer is reset.
   * - ``onRunPause(bool paused)``
     - background
     - Called directly from the background reader when a DAS
       pause/resume annotation is received.  ``paused = true`` for
       PAUSE, ``false`` for RESUME.
   * - ``onBeforeExtract()``
     - foreground
     - Called at the very start of ``extractData()``, before
       ``doExtractData()``.  Override here for any per-extraction
       bookkeeping that does not fit the run-boundary hook model.
   * - ``onAfterExtract()``
     - foreground
     - Called at the end of ``extractData()``, but **only** when
       ``doExtractData()`` returned normally (i.e., did not throw).
       Override here for success-only bookkeeping — state that
       must not take effect when extraction is interrupted by
       ``Exception::NotYet``.

The asymmetric dispatch is intentional:

* **Run-state transitions are deferred** to the next ``extractData()``
  call because they have workspace-level side effects (cache clears,
  re-init) that must run on the foreground thread.
* **Pause/resume is applied immediately** because it has no
  workspace-level side effect — it only gates event appending in
  ``rxPacket(BankedEventPkt)``.  Deferring pause/resume to
  ``extractData()`` would retroactively mis-categorise events that
  arrived between the PAUSE annotation and the next extraction.
* **BeginRun and EndRun are committed at different phases.**
  ``BeginRun`` is dispatched from ``onBeforeExtract()`` so the new
  run's workspace initialisation is in place before
  ``doExtractData()`` snapshots it.  ``EndRun`` is dispatched from
  ``onAfterExtract()`` so ``doExtractData()`` can first harvest the
  finishing run's accumulated events before ``onEndRun()`` resets the
  buffer.

All hooks have no-op default implementations in ``LiveListener`` and
may be selectively overridden.


Migration recipe for listener authors
--------------------------------------

If you maintain a concrete ``ILiveListener`` subclass, migrate as
follows.

Step 1 — Remove your ``runStatus()`` override
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The base-class shim is correct for almost every listener.  Drop the
override entirely; the deprecation warning at the call sites will
prompt eventual migration to ``lastTransition()`` / ``runState()``.

Step 2 — Add a ``listenerState() const override``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``listenerState()`` is **pure virtual**.  The minimal override maps
your existing connection flag to the enum:

.. code-block:: cpp

   API::ListenerState listenerState() const override {
       return m_isConnected ? API::ListenerState::Connected
                            : API::ListenerState::Disconnected;
   }

A listener that has back-pressure semantics (e.g. SNS's
``m_pauseNetRead``) or that can report an error from the background
thread should expand the override to return ``ReadWait`` / ``Error``
as appropriate.

Step 3 — Override ``runState() const`` (recommended)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The base default of ``NoRun`` is rarely what you want.  Provide an
explicit override even if it is a one-liner returning ``Running``:

.. code-block:: cpp

   RunStatus runState() const override { return API::ILiveListener::Running; }

Step 4 — Optionally override ``isPaused()`` and ``lastTransition()``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The base defaults (``false`` / ``nullopt``) are correct for listeners
without pause semantics or run-boundary edge detection.

Step 5 — Override ``doExtractData()``, not ``extractData()``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If your previous override was

.. code-block:: cpp

   std::shared_ptr<Workspace> MyListener::extractData() override { ... }

mechanically rename it to ``doExtractData()``.  ``extractData()`` is
``final`` on ``API::LiveListener`` and the compiler will reject the
old override.

Step 6 — Move any FSM-tick code out of ``runStatus()``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If your old ``runStatus()`` contained mutating logic (incrementing
counters, advancing timers, performing I/O), move it to a private
helper and call that helper from ``onBeforeExtract()``.  If the
bookkeeping is success-only — state that must not take effect when
``doExtractData()`` throws ``Exception::NotYet`` — use
``onAfterExtract()`` instead.  See the worked example for
``FakeEventDataListener`` below.


Worked examples
---------------

The following worked examples cover every concrete
``ILiveListener`` subclass in the tree.  ``SNSLiveEventDataListener``
is treated separately in :ref:`SNSLiveEventDataListenerRefactoring`.

Pattern A — constant run state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Used by ``FileEventDataListener``, ``ISISLiveEventDataListener``,
``ISISHistoDataListener``, ``TestGroupDataListener``,
``TestDataListener``, and the unit-test ``MockLiveListener``.  These
listeners have no notion of run boundaries; their old ``runStatus()``
returned a constant.

.. code-block:: cpp

   // Header
   RunStatus       runState()      const override { return Running; }
   ListenerState   listenerState() const override {
       return m_isConnected ? ListenerState::Connected
                            : ListenerState::Disconnected;
   }

   // Drop the existing runStatus() override; the base-class default is
   // equivalent and reports the new edge-detection contract correctly
   // (always nullopt for these listeners).

``FileEventDataListener`` additionally maps its chunk index to the
connection state:

.. code-block:: cpp

   ListenerState listenerState() const override {
       return m_chunkNumber > 0 ? ListenerState::Connected
                                : ListenerState::Disconnected;
   }

Pattern B — edge-detected run state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Used by ``KafkaEventListener`` and ``KafkaHistoListener``.  These
listeners query an underlying decoder to determine whether an
end-of-run boundary has been observed, but they do not maintain a
``BeginRun`` / ``EndRun`` edge-detection contract — only "are we
inside a run right now?".

.. code-block:: cpp

   RunStatus runState() const override {
       return m_decoder->hasReachedEndOfRun() ? EndRun : Running;
   }
   ListenerState listenerState() const override {
       return m_decoder && m_decoder->isCapturing()
                ? ListenerState::Connected
                : ListenerState::Disconnected;
   }

``lastTransition()`` is not overridden (returns ``nullopt``); Kafka
does not have the ``BeginRun`` / ``EndRun`` edge-detection contract
that ADARA does, so the deprecated ``runStatus()`` shim falls through
to ``runState()`` and returns the same value the legacy override
would have.

Pattern C — FSM-tick anti-pattern fix
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Two listeners besides ``SNSLiveEventDataListener`` had real side
effects inside ``runStatus()`` — exactly the anti-pattern this
refactor exists to eliminate.

**``FakeEventDataListener``** (pre-refactor):

.. code-block:: cpp

   ILiveListener::RunStatus FakeEventDataListener::runStatus() {
       if (m_endRunEvery > 0 &&
           DateAndTime::getCurrentTime() > m_nextEndRunTime) {
           m_nextEndRunTime = DateAndTime::getCurrentTime() + m_endRunEvery;
           m_runNumber++;
           return EndRun;
       } else {
           return Running;
       }
   }

This getter incremented ``m_runNumber`` and advanced
``m_nextEndRunTime`` as a side effect of being polled.

**Post-refactor.**  All end-of-run side effects move to
``onAfterExtract()``; ``runState()`` becomes a pure getter.
``onBeforeExtract()`` is not overridden (the base no-op suffices):

.. code-block:: cpp

   // Header
   RunStatus runState() const override               { return m_runState; }
   ListenerState listenerState() const override      {
       return m_isConnected ? ListenerState::Connected
                            : ListenerState::Disconnected;
   }
   std::optional<RunStatus> lastTransition() const override { return m_lastTransition; }

   protected:
   void onAfterExtract() override;

   private:
   RunStatus m_runState{Running};
   std::optional<RunStatus> m_lastTransition;

   // Implementation
   void FakeEventDataListener::onAfterExtract() {
       // This listener only ever commits an EndRun edge (never BeginRun), so
       // the simple pattern — unconditional reset then conditional re-set in
       // the same onAfterExtract() — is correct here.  Listeners that also
       // commit a BeginRun edge in onBeforeExtract() must use the gated-clear
       // pattern (m_previousExtractCompleted) described in the C1 invariant
       // section below, so that the BeginRun edge survives through the return
       // of the same extractData() call and is observable by the caller.
       m_lastTransition.reset();
       if (m_endRunEvery > 0 &&
           DateAndTime::getCurrentTime() > m_nextEndRunTime) {
           m_nextEndRunTime = DateAndTime::getCurrentTime() + m_endRunEvery;
           m_runNumber++;
           m_runState       = EndRun;
           m_lastTransition = EndRun;
       } else {
           m_runState = Running;
       }
   }

The externally observable cadence of ``EndRun`` and the
``m_runNumber`` increment is preserved; only the trigger moves from
``runStatus()`` polling to ``extractData()`` invocation.  Placing all
side effects in ``onAfterExtract()`` rather than ``onBeforeExtract()``
is required by the :ref:`C1 invariant <migration-c1-invariant>`:
``onAfterExtract()`` runs only when ``doExtractData()`` returns
normally, so a ``NotYet`` thrown from ``doExtractData()`` cannot erase
or advance state that the retry will need.  Note that this pattern is
sufficient when the listener only commits edges in ``onAfterExtract()``
(as ``FakeEventDataListener`` does); see the C1 section for the
additional ``m_previousExtractCompleted`` flag needed when edges are
also committed in ``onBeforeExtract()``.

**``SINQHMListener``** (pre-refactor): ``runStatus()`` performed an
HTTP request, parsed the response, wrote the ``hmhost`` member, set
``dimDirty`` when transitioning ``NoRun → Running``, and was *also*
invoked from inside ``extractData()`` specifically to perform these
side effects.

**Post-refactor.**  Extract the HTTP poll into a private
``pollStatus()`` helper; have ``onBeforeExtract()`` call it.
``runState()`` returns the cached state:

.. code-block:: cpp

   // Header
   RunStatus runState() const override         { return m_cachedRunState; }
   ListenerState listenerState() const override {
       return connected ? ListenerState::Connected
                        : ListenerState::Disconnected;
   }

   protected:
   void onBeforeExtract() override { pollStatus(); }

   private:
   void pollStatus();              // contains the HTTP request and field updates
   RunStatus m_cachedRunState{NoRun};

   // pollStatus() body is the old runStatus() body, but writes
   // m_cachedRunState instead of returning, and continues to write
   // hmhost and dimDirty as before.

Because ``extractData()`` already triggered ``pollStatus()`` implicitly
in the old code (it called the side-effecting ``runStatus()``), callers
that only invoke ``extractData()`` see no behavioural change.

.. note::

   Unlike the ``FakeEventDataListener`` example above,
   ``SINQHMListener`` writes its cached run-state and the ``dimDirty``
   flag inside ``onBeforeExtract()`` (via ``pollStatus()``) rather than
   ``onAfterExtract()``.  This is correct **only** because (a)
   ``doExtractData()`` does not throw ``Exception::NotYet`` and (b) the
   listener does not override ``lastTransition()``.
   ``doExtractData()`` *reads* ``dimDirty`` and ``hmhost`` to build the
   workspace, so the poll must precede the build — that is the
   load-bearing reason the call lives in ``onBeforeExtract()``.  If a
   future change to this listener adds a ``NotYet`` path *or* a
   ``lastTransition()`` override, the ``NoRun → Running`` side effects
   (``dimDirty`` set, ``oldStatus`` advance) must move to
   ``onAfterExtract()`` to satisfy the
   :ref:`C1 invariant <migration-c1-invariant>`.

Pattern D — test mocks
~~~~~~~~~~~~~~~~~~~~~~

``MockLiveListener`` and similar fixtures need only the two-line
addition:

.. code-block:: cpp

   class MockLiveListener : public ILiveListener {
   public:
       RunStatus runState() const override { return Running; }
       ListenerState listenerState() const override {
           return ListenerState::Connected;
       }
       // ... other mocked methods ...
   };

Any algorithm-level mocks (in ``LoadLiveDataTest``,
``MonitorLiveDataTest``, ``StartLiveDataTest``) that derive from
``ILiveListener`` directly need the same two-line addition.


Behaviour preservation guarantees
---------------------------------

The refactor preserves every observable algorithm-level behaviour
except one: stand-alone ``LoadLiveData`` previously deadlocked after a
run boundary and now succeeds.  That is the bug the refactor exists to
fix.

.. list-table::
   :header-rows: 1
   :widths: 55 22 23

   * - Behaviour
     - Pre-refactor
     - Post-refactor
   * - ``runStatus()`` returns ``BeginRun`` / ``EndRun`` exactly once
       at each boundary
     - yes, via mutation
     - yes, via ``lastTransition()`` populated by ``extractData()``
   * - Workspace re-initialised at run boundaries
     - inside ``runStatus()``
     - ``onBeginRun()`` called from ``onBeforeExtract()``;
       ``onEndRun()`` called from ``onAfterExtract()``
   * - ``MonitorLiveData`` workspace renaming triggers on
       ``BeginRun`` / ``EndRun``
     - yes
     - yes (via the legacy ``runStatus()`` shim)
   * - Listener can be queried for its state without mutating it
     - **no**
     - yes (``runState()``, ``isPaused()``, ``listenerState()``,
       ``lastTransition()`` all ``const``)
   * - Stand-alone ``LoadLiveData`` produces a workspace
     - **no** (deadlocks)
     - yes

.. _migration-c1-invariant:

Critical detail (C1)
~~~~~~~~~~~~~~~~~~~~

``m_lastTransition`` must survive ``Exception::NotYet`` retries so
that ``LoadLiveData``'s retry loop (``LoadLiveData.cpp:476–490``) does
not lose a committed edge.  This is the **C1 invariant**.

For listeners that only commit edges in ``onAfterExtract()`` (e.g.
``FakeEventDataListener``), the simple pattern — unconditional reset at
the top of ``onAfterExtract()``, then conditional re-set — is
sufficient: because ``onAfterExtract()`` is only called on the success
path, a ``NotYet`` from ``doExtractData()`` bypasses the clear
entirely.

For listeners that also commit a ``BeginRun`` edge in
``onBeforeExtract()`` (e.g. ``SNSLiveEventDataListener``), an
additional guard is required: a ``m_previousExtractCompleted`` flag
that is set to ``true`` only at the end of ``onAfterExtract()`` and is
consumed (cleared) at the start of the next ``onBeforeExtract()`` after
both ``NotYet`` gates pass.  This ensures that:

* **The edge is observable after the extract that committed it.**  Both
  ``BeginRun`` and ``EndRun`` are visible via ``lastTransition()``
  between the ``extractData()`` that committed them and the next
  successful ``extractData()``.  Callers such as ``MonitorLiveData``
  read ``lastTransition()`` *after* ``extractData()`` returns and rely
  on the edge being present (e.g. to drive the ``_post``
  workspace-rename branch for ``BeginRun``).
* **The edge survives retries.**  A ``NotYet`` thrown from Phase 1
  (before either gate) or Phase 2/3 leaves
  ``m_previousExtractCompleted == false``, so the retry's
  ``onBeforeExtract()`` skips the clear.

Two subtleties to flag in code review
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. **``m_lastTransition`` lifetime.**  ``m_lastTransition`` must not be
   cleared on a ``NotYet`` retry path, and it must be observable to
   callers (e.g. ``MonitorLiveData``) after the ``extractData()`` that
   committed it returns.  The SNS listener satisfies both with
   ``m_previousExtractCompleted``: the clear is deferred to the start
   of the *next* ``onBeforeExtract()`` after both ``NotYet`` gates
   pass, and it only fires when the flag is ``true`` (meaning the
   previous ``extractData()`` ran ``onAfterExtract()`` to completion).
   ``BeginRun`` and ``EndRun`` are therefore symmetric: both are
   visible via ``lastTransition()`` from the moment ``extractData()``
   returns until the following successful ``extractData()``.  A caller
   that never calls ``extractData()`` never sees an edge.

2. **Pause/resume packets are processed in the background thread.**
   ``onRunPause()`` is called directly from the background reader,
   not deferred to ``extractData()``.  This is intentional: applying
   ``m_isDasPaused`` immediately at the annotation packet boundary
   gives accurate event filtering — events arriving between the
   ``PAUSE`` annotation and the next ``extractData()`` call are
   correctly discarded.  Deferring to ``extractData()`` would
   retroactively mis-categorise those events.


Pitfalls and FAQ
----------------

Why is ``extractData()`` ``final``?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

So that ``onBeforeExtract()`` is *guaranteed* to run before any
workspace construction, on every code path, for every listener.  This
guarantee is what makes stand-alone ``LoadLiveData`` work: it does
not need to know that some listeners require an explicit "commit"
step.

What if my listener never had a ``runStatus()`` override?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Then only Step 2 from the migration recipe is required: add a
``listenerState() const override``.  The rest of the new API will fall
through to the safe defaults.

Why are ``onBeginRun`` / ``onEndRun`` protected rather than public?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Exposing them publicly would re-violate objective (3) above by letting
external code drive the FSM.  Keeping them protected makes them
explicit and individually testable (subclass the listener in a unit
test, override the hook, observe invocations) without giving callers a
foothold to commit transitions out of band.

How do I drive ``onRunPause`` in a unit test?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Either feed an ``AnnotationPkt(PAUSE)`` directly into ``rxPacket()``
(SNS), or — for listeners with a simpler pause model — subclass the
listener and call ``onRunPause(true/false)`` from a test fixture
method.  The hook is ``protected virtual`` precisely so test fixtures
can reach it.

When should I override ``onAfterExtract()`` rather than ``onBeforeExtract()``?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use ``onAfterExtract()`` for bookkeeping that is meaningful only when
``doExtractData()`` returned a workspace successfully — work that must
*not* take effect when extraction is interrupted by
``Exception::NotYet``.  Both ``SNSLiveEventDataListener`` and
``FakeEventDataListener`` override this hook.

If your listener only commits edges in ``onAfterExtract()`` (as
``FakeEventDataListener`` does), the simple reset-then-re-set pattern
in that hook is sufficient.

If your listener commits a ``BeginRun`` edge in ``onBeforeExtract()``,
it must also use the ``m_previousExtractCompleted`` guard (set at the
end of ``onAfterExtract()``, consumed at the start of the next
``onBeforeExtract()`` after the ``NotYet`` gates pass) so that the edge
survives retries and is observable to callers after ``extractData()``
returns.  See the :ref:`C1 invariant <migration-c1-invariant>` and the
``SNSLiveEventDataListener`` implementation for the full pattern.


Further reading
---------------

* :ref:`SNSLiveEventDataListenerRefactoring` — the SNS-specific
  design (ADARA packet flow, state-machine diagrams, deferred
  ``RunStatusPkt`` handling, the pre-existing ``m_ignorePackets``
  defect).
* ``Framework/API/inc/MantidAPI/ILiveListener.h`` — authoritative
  interface declaration.
* ``Framework/API/inc/MantidAPI/LiveListener.h`` — ``extractData()``
  template method and default hook implementations.
* ``Framework/API/src/ILiveListener.cpp`` — default ``runStatus()``
  shim.
