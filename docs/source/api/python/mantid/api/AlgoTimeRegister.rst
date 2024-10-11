==================
 AlgoTimeRegister
==================

This is a Python binding to the C++ class Mantid::Instrumentation::AlgoTimeRegister.

*bases:* :py:obj:`mantid.api.AlgoTimeRegisterImpl`

.. module:`mantid.api`

.. autoclass:: mantid.api.AlgoTimeRegisterImpl
    :members:
    :undoc-members:
    :inherited-members:

------
Usage
------

In the beggining of the script, initialize :py:obj:`~mantid.api.AlgoTimeRegister` to start the clock (START_POINT).
Then use python `time` (time.time_ns()) to measure time (start_time and end_time in nanoseconds) and a function (name).
Afterwards, call :py:obj:`~mantid.api.AlgoTimeRegister.addTime()` to store
them in the `performancelog.filename` log file when `performancelog.write` is activated (defined at `Mantid.user.properties` file)
in the following format:

ThreadID=<thread_id>, AlgorithmName=<name>, StartTime=<start_time>, EndTime=<end_time>

**Example 1 - Use AlgoTimeRegister to measure time - default case:**
This example shows the initialization process and adding a time entry

.. testcode:: AlgoTimeRegisterStartAddDefault

    # use python time module to measure time in nanoseconds
    import time

    #initialize the clock *once*
    AlgoTimeRegister.Instance()

    # get start time in nanoseconds before calling the function of interest
    start_time = time.time_ns()

    # execute the function
    demo_function()

    # get end time in nanoseconds after it is completed
    end_time = time.time_ns()

    # store the time log
    name = "demo_function"
    AlgoTimeRegister.addTime(name,start_time,end_time)


Example Output at `Mantid.user.properties.performance.filename` file:

.. testoutput:: AlgoTimeRegisterStartAddDefault

    START_POINT: 1728507408768867137 MAX_THREAD: 12
    ThreadID=124522639565888, AlgorithmName=demo_function, StartTime=144539, EndTime=1346405816

**Example 2 - Use AlgoTimeRegister to measure time - overwrite the performance.log file:**
The performance configuration can be overwritten in the script, by defining a custom `performancelog.filename`
and calling evertyhing within the `amend_config` with block.

.. testcode:: AlgoTimeRegisterStartOverWriteFile

    #define `performancelog.filenamer` filepath for ta particular script
    performance_config = {"performancelog.write": "On", "performancelog.filename": <custom filepath>}
    with amend_config(**performance_config):
        # use python time module to measure time in nanoseconds
        import time

        #initialize the clock *once*
        AlgoTimeRegister.Instance()

        # get start time in nanoseconds before calling the function of interest
        start_time = time.time_ns()

        # execute the function
        demo_function()

        # get end time in nanoseconds after it is completed
        end_time = time.time_ns()

        # store the time log
        name = "demo_function"
        AlgoTimeRegister.addTime(name,start_time,end_time)


Example Output at `performance_config.performance.filename` file:

.. testoutput:: AlgoTimeRegisterStartOverWriteFile

    START_POINT: 1728507555711066754 MAX_THREAD: 12
    ThreadID=139238466401344, AlgorithmName=demo_function, StartTime=144981, EndTime=4115162290

**Example 3 - Use AlgoTimeRegister to measure time - initialization omitted:**
The :py:obj:`~mantid.api.AlgoTimeRegister` initialization can be omitted,
if a Mantid algorithm is the first command in the script and the time before this is not needed.
In any other case, initialization should be specified very eary on, in the script.

.. testcode:: AlgoTimeRegisterStartWorkspace

    # use python time module to measure time in nanoseconds
    import time

    #call algorithm, clock initialization
    #internally it calls AlgoTimeRegister.addTime
    ws = CreateSampleWorkspace()

    # get start time in nanoseconds before calling the function of interest
    start_time = time.time_ns()

    # execute the function
    demo_function()

    # get end time in nanoseconds after it is completed
    end_time = time.time_ns()

    # store the time log
    name = "demo_function"
    AlgoTimeRegister.addTime(name,start_time,end_time)

Example Output at `Mantid.user.properties.performance.filename` file:

.. testoutput:: AlgoTimeRegisterStartWorkspace

    START_POINT: 1728507650978221781 MAX_THREAD: 12
    ThreadID=138583991714880, AlgorithmName=CreateSampleWorkspace, StartTime=154046, EndTime=1818067
    ThreadID=138583991714880, AlgorithmName=demo_function, StartTime=2044656, EndTime=4117415173