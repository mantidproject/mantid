.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves a TableWorkspace at least 8 columns wide into an ASCII file in 17-column Reflectometry TBL format compatible with the old ISIS Reflectometry Interface. The columns must be typed: ``str, str, str, str, str, str, str, int``

The 8 columns are grouped into rows of 17 according to stitch index, so up to 3 rows in the table would become a single row in the TBL file like so: (Where Z is an identical stitch group index, and - is ignored as only the first instance of P and Q are used in the file)

::

    A, B, C, D, E, P, Q, Z
    F, G, H, I, J, -, -, Z
    K, L, M, N, O, -, -, Z

becomes
::

    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q

Limitations
###########

The Algorithm will fail if any stitch index appears more than 3 times, as the old interface does not support more than 3 runs per row.

Stitch groups of index 0 are treated as non-grouped, and will not be grouped with one another (and by extension can be larger than 3 members). They will however be moved to the end of the file

Any columns after the eighth are ignored

Usage
-----

**Example - Save a TableWorkspace in Reflectometry TBL format**

.. testcode:: ExReflTBLSimple

    #import the os path libraries for directory functions
    import os

    # Create a table workspace with data to save. You'd normally have the table workspace in mantid already, probably as a product of LoadReflTBL
    ws = CreateEmptyTableWorkspace()
    ws.addColumn("str","Run(s)");
    ws.addColumn("str","ThetaIn");
    ws.addColumn("str","TransRun(s)");
    ws.addColumn("str","Qmin");
    ws.addColumn("str","Qmax");
    ws.addColumn("str","dq/q");
    ws.addColumn("str","Scale");
    ws.addColumn("int","StitchGroup");
    nextRow = {'Run(s)':"13460",'ThetaIn':"0.7",'TransRun(s)':"13463,13464",'Qmin':"0.01",'Qmax':"0.06",'dq/q':"0.04",'Scale':"",'StitchGroup':1}
    ws.addRow ( nextRow )
    nextRow = {'Run(s)':"13470",'ThetaIn':"2.3",'TransRun(s)':"13463,13464",'Qmin':"0.035",'Qmax':"0.3",'dq/q':"0.04",'Scale':"",'StitchGroup':0}
    ws.addRow ( nextRow )
    nextRow = {'Run(s)':"13462",'ThetaIn':"2.3",'TransRun(s)':"13463,13464",'Qmin':"0.035",'Qmax':"0.3",'dq/q':"0.04",'Scale':"",'StitchGroup':1}
    ws.addRow ( nextRow )
    nextRow = {'Run(s)':"13469",'ThetaIn':"0.7",'TransRun(s)':"13463,13464",'Qmin':"0.01",'Qmax':"0.06",'dq/q':"0.04",'Scale':"",'StitchGroup':2}
    ws.addRow ( nextRow )

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "ReflTBLFile.tbl")

    # perform the algorithm
    SaveReflTBL(Filename=savefile,InputWorkspace=ws)

    #the file contains
    # 13460,0.7,"13463,13464",0.01,0.06,13462,2.3,"13463,13464",0.035,0.3,,,,,,0.04,
    # 13469,0.7,"13463,13464",0.01,0.06,,,,,,,,,,,0.04,
    # 13470,2.3,"13463,13464",0.035,0.3,,,,,,,,,,,0.04,
    print "File Exists:", os.path.exists(savefile)

.. testcleanup:: ExReflTBLSimple

    os.remove(savefile)

Output:

.. testoutput:: ExReflTBLSimple

    File Exists: True

.. categories::
