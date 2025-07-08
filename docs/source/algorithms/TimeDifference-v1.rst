.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes a list of workspace names (workspaces can be :ref:`Matrix <MatrixWorkspace>` or :ref:`Group <WorkspaceGroup>`)
and compares the middle time (defined below) at which each run occurs to a reference value.
The differences are entered into the output table workspace with columns for the time in seconds and hours (plus errors).
To establish the time at which each run occurs, the start times are taken from either ``run_start`` or ``start_time`` logs and the end times
from ``run_end`` or ``end_time`` logs.
For each workspace, the run duration is computed as : :math:`duration = end_{time} - start_{time}` and the time as the middle
time of the duration interval: :math:`midtime = start_{time} + duration/2`.
Then, each difference time is computed by subtracting the calculated ``midtime`` stamp from the reference. :math:`difference = midtime - midtime_{ref}` .
Taking the duration as the absolute error of each ``midtime`` stamp, the error of the difference is computed as the sum of each ``midtime`` error.

*  If a reference workspace is not provided, the first workspace in the Workspaces property list will be set as the reference.
*  For group workspaces, start and end time stamps for all the workspaces contained in the group will be extracted, and the duration interval
   computed from the runs with the earliest and latest start and end times, respectively.


Usage
-----

** Example **

.. testcode:: ExTimeDifferences

    time_origin = np.datetime64('2025-06-12T08:00:00.000000000')
    experiment_duration = np.timedelta64(60,'s')

    start_times = [0,100,200,300]

    names = []
    for start in start_times:
        names.append(str(start))
        start_time = time_origin + np.timedelta64(start,'s')
        end_time = start_time + experiment_duration
        CreateWorkspace(OutputWorkspace=names[-1], DataX = 0 , DataY = 1)
        AddSampleLog(Workspace=names[-1], LogName='start_time', LogText=str(start_time), LogType='String')
        AddSampleLog(Workspace=names[-1], LogName='end_time', LogText=str(end_time), LogType='String')

    table = TimeDifference(InputWorkspaces=names)
    print("Time Differences are, in seconds: " + str(table.column(2)))

Output:

.. testoutput:: ExTimeDifferences

    Time Differences are, in seconds: [0.0, 100.0, 200.0, 300.0]

.. categories::

.. sourcelink::
