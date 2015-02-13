.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm to control jobs running on the SCARF computer cluster at
RAL, STFC (see http://www.scarf.rl.ac.uk/ for more information). This
algorithm can be used to log in and out from the cluster, and to
initiate, query the status of, or cancel a job. It has been introduced
to control tomographic reconstruction jobs but in principle it can be
used for any other task.

In a typical use case or session you would use the algorithm a first
time to login (for which you need to select the 'LogIn' action and set
the username and password fields). After this step you can use the
algorithm again several times, to submit jobs (setting the action
'SubmitJob'), query the status of the jobs running on the computer
cluster (setting the action to 'JobStatus' or 'JobStatusByID'), cancel
jobs (setting the action 'CancelJob') and log out from the cluster
(action 'LogOut'). You can also upload and download files. After
logging out, subsequent submit or status queries will fail with an
informative message. Note that the server will log out users every
undetermined amount of time, which depends on server settings.

In principle, in a simple use case, the same username will be used in
all the calls to this algorithm. This means that you type in the
username only the first time that you use this algorithm, as it will
be remembered and filled in automatically in the next calls.  But note
that it is possible to change the username passed to the algorithm
every time you call the algorithm. This means that you can use this
algorithm to control jobs for multiple users simultaneously from the
same instance of Mantid. You can use for example use the username
associated to a particular project or instrument, and in parallel your
personal username for different jobs (which can be useful to
distinguish quick tests, calibration of parameters, etc.). For this to
work, you of course need to perform a 'LogIn' action for every
username that is used in submit or query status actions.

The 'JobStatus' and 'JobStatusByID' actions produce an output
`TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_ with
status and general information about the jobs as retrieved from the
compute resource/server.

The algorithm relies on a web service provided by the SCARF computer
cluster in order to control jobs on the cluster. If you use this
algorithm from its dialog (for example starting it from the algorithm
explorer in Mantid) the password will not be shown on the screen. This
algorithm can be used interactively. In such case, it is absolutely
not recommended to call it from scripts or the Python script
interpreter window, as you will be passing passwords as plain text.

The alternative ways to monitor and control your jobs are via shell
login and via the web platform at https://portal.scarf.rl.ac.uk/.

This algorithm is used by other components of Mantid, in particular
the custom graphical interface for tomography (IMAT instrument).

Usage
-----

**Example**

.. testcode:: SCARFTomoReconstruction

    try:
        SCARFTomoReconstruction(UserName='foouser', Action='Login')
    except ValueError:
        print "ValueError, as expected, because no Password= parameter given"

    try:
        SCARFTomoReconstruction(UserName='foouser', Action='Submit')
    except ValueError:
        print "ValueError, as expected, as it was not previously logged on"

Output:

.. testoutput:: SCARFTomoReconstruction

   ValueError as expected because no Password= parameter given
   ValueError, as expected, as it was not previously logged on

.. categories::
