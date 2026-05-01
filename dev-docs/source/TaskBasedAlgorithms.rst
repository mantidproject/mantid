.. _TaskBasedAlgorithms:

=====================
Task Based Algorithms
=====================

Overview
--------

A ``TaskBasedAlgorithm`` is a C++ helper type for writing workflow-style algorithms as a sequence of
small, named tasks.

This is useful when the caller needs control over execution flow while reusing the same implementation.

The algorithm exposes a ``TaskExecutionOrder`` property so users can choose the task order at runtime.
The ``TaskBasedAlgorithm`` then stages those tasks and executes them in that order.

Concepts
--------

``TaskBasedAlgorithm<T>``
~~~~~~~~~~~~~~~~~~~~~~~~~

Derive your algorithm from ``TaskBasedAlgorithm<YourAlgorithm>``.
The base type provides task staging, dependency evaluation, output tracking and final
``OutputWorkspace`` selection.

``AlgorithmTask``
~~~~~~~~~~~~~~~~~

A task is a nested class deriving from ``AlgorithmTask``.
Each task:

* has a unique task name,
* declares expected outputs,
* declares one or more dependency sets,
* implements ``executeImpl()``.

Dependency sets
~~~~~~~~~~~~~~~

A dependency set is a valid group of upstream task outputs.
If one set can be fulfilled, the task may run.
This allows tasks to support alternative upstream paths.

For example, a task can depend on:

* ``TaskC`` output, **or**
* ``TaskB`` output.

If both are available, the active set is chosen based on proximity in ``TaskExecutionOrder``.

Selected output
~~~~~~~~~~~~~~~

A task can produce more than one output workspace.
The task's *selected output* is the one propagated to the algorithm-level ``OutputWorkspace``
when that task is last in the staged order.

How task execution works
------------------------

At a high level:

1. ``initTaskBasedAlgorithm`` registers task types and declares ``TaskExecutionOrder``.
2. ``configureAlgorithmTasks`` validates and stages tasks in user-specified order.
3. The first staged task is seeded with ``InputWorkspace``.
4. Each task evaluates whether at least one dependency set is fulfillable.
5. The task runs ``executeImpl()`` and publishes outputs.
6. After the final task, the selected output is written to ``OutputWorkspace``.

Writing a task based algorithm
------------------------------

1. Derive from ``TaskBasedAlgorithm``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

  class MyWorkflow final : public Mantid::API::TaskBasedAlgorithm<MyWorkflow> {
  public:
    const std::string name() const override { return "MyWorkflow"; }
    const std::string summary() const override { return "Example task-based workflow"; }
    int version() const override { return 1; }

    void init() override {
      declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                          "InputWorkspace", "", Mantid::Kernel::Direction::Input),
                      "Input workspace");
      declareProperty(std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                          "OutputWorkspace", "", Mantid::Kernel::Direction::Output),
                      "Final output workspace");

      initTaskBasedAlgorithm<TaskA, TaskB, TaskC, TaskD>(
          {"TaskA", "TaskB", "TaskC", "TaskD"});
    }

    void exec() override { execTasks(); }

    class TaskA; class TaskB; class TaskC; class TaskD;
  };

2. Define tasks and dependencies
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The examples below are based on ``TaskBasedAlgorithmTest.h``.

How to set task dependencies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use ``setDependantTask`` to require an upstream output.
Use ``addDependantTaskSet`` to support alternative dependency sets.

.. code-block:: cpp

  class TaskB final : public AlgorithmTask {
  public:
    explicit TaskB(MyWorkflow *parent) : AlgorithmTask(parent, "TaskB") {
      setExpectedOutputs({"TaskBOutput1", "TaskBOutput2"});

      // Set 0: depend on TaskA output
      setDependantTask("TaskA", "TaskAOutput", "InputWorkspace");

      // Set 1: alternative dependency on TaskC
      auto altSet = addDependantTaskSet();
      setDependantTask("TaskC", "TaskCOutput", "InputWorkspace", altSet);
    }

    void executeImpl() override {
      auto inputWS = getDependantWorkspace("InputWorkspace");
      // ... do work
    }
  };

If no output name is supplied (``setDependantTask("TaskD")``), all expected outputs from
that upstream task are made available using their output names as aliases.

How to set task outputs
^^^^^^^^^^^^^^^^^^^^^^^

Declare expected outputs with ``setExpectedOutputs`` and publish each output via ``outputWorkspace``.

.. code-block:: cpp

  class TaskD final : public AlgorithmTask {
  public:
    explicit TaskD(MyWorkflow *parent) : AlgorithmTask(parent, "TaskD") {
      setExpectedOutputs({"TaskDOutput1", "TaskDOutput2"});
      setSelectedOutput("TaskDOutput2");
    }

    void executeImpl() override {
      auto ws1 = /* create output */;
      auto ws2 = /* create output */;
      outputWorkspace(ws1, "TaskDOutput1");
      outputWorkspace(ws2, "TaskDOutput2");
    }
  };

How to specify a default ``TaskExecutionOrder``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Pass a default vector to ``initTaskBasedAlgorithm`` in ``init()``.
Users can still override this property.

.. code-block:: cpp

  initTaskBasedAlgorithm<TaskA, TaskB, TaskC, TaskD>(
      {"TaskA", "TaskB", "TaskC", "TaskD"});

Optionally override ``constructTaskExecutionOrder()`` if the default order must be assembled dynamically.

Validation and behaviour notes
------------------------------

* Invalid task names in ``TaskExecutionOrder`` are rejected.
* Duplicate task names in ``TaskExecutionOrder`` are rejected.
* By default, the input workspace is cloned before task execution.
  Use ``setMutableInput(true)`` if in-place mutation is intended.

Future work
-----------

The following improvements are planned/possible extensions:

* Allow repeated task names in ``TaskExecutionOrder``, if certain tasks can be executed more than once.
* Conduct more validation of the dependency structure at initialization as opposed to run-time.
* Consider formally building in conditional skipping of specified tasks.
* Explore parallel execution of independent task branches.
* Ultimately, it would be great to have a flowchart-like GUI allowing users to specify the flow of tasks.

See also
--------

* ``Framework/API/inc/MantidAPI/TaskBasedAlgorithm.h``
* ``Framework/API/test/TaskBasedAlgorithmTest.h``
