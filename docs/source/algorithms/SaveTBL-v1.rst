.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a TableWorkspace into an ASCII file in TBL format. All of the columns must be typed as `str` apart from the Group column which must be of type `int`.

The following TableWorkspace:

+------+-----+-----+-----+------+------+----------------------------+
|Run   |Angle|QMin |QMax |Energy|Group |Options                     |
+======+=====+=====+=====+======+======+============================+
|14455 |1.5  |0.333|1.44 |1.4   |1     |                            |
+------+-----+-----+-----+------+------+----------------------------+
|14456 |1.5  |0.14 |3.522|1.6   |1     |ProcessingInstructions="1:2"|
+------+-----+-----+-----+------+------+----------------------------+
|14457 |1.5  |0.83 |2.44 |1.2   |2     |                            |
+------+-----+-----+-----+------+------+----------------------------+

becomes

::

    Run,Angle,Qmin,Qmax,Energy,Group,Options
    14455,1.5,0.333,1.44,1.4,1,
    14456,1.5,1.4,3.522,1.6,1,ProcessingInstructions="1:2"
    14457,1.5,0.83,2.44,1.2,2,

Limitations
###########

The Algorithm will fail if any of the columns is not of type 'str'.

Usage
-----

**Example - Save a TableWorkspace in Reflectometry TBL format**

.. testcode:: ExTBLSimple

    #import the os path libraries for directory functions
    import os

    # Create a table workspace with data to save. You'd normally have the table workspace in mantid already, probably as a product of LoadTBL
    ws = CreateEmptyTableWorkspace()
    ws.addColumn("str","Run(s)");
    ws.addColumn("str","ThetaIn");
    ws.addColumn("str","TransRun(s)");
    ws.addColumn("str","Qmin");
    ws.addColumn("str","Qmax");
    ws.addColumn("str","dq/q");
    ws.addColumn("str","Scale");
    ws.addColumn("str","StitchGroup");
    ws.addColumn("str","Options");
    nextRow = {'Run(s)':"13460",'ThetaIn':"0.7",'TransRun(s)':"13463+13464",'Qmin':"0.01",'Qmax':"0.06",'dq/q':"0.04",'Scale':"",'StitchGroup':"1",'Options':""}
    ws.addRow ( nextRow )
    nextRow = {'Run(s)':"13470",'ThetaIn':"2.3",'TransRun(s)':"13463+13464",'Qmin':"0.035",'Qmax':"0.3",'dq/q':"0.04",'Scale':"",'StitchGroup':"0",'Options':""}
    ws.addRow ( nextRow )
    nextRow = {'Run(s)':"13462",'ThetaIn':"2.3",'TransRun(s)':"13463+13464",'Qmin':"0.035",'Qmax':"0.3",'dq/q':"0.04",'Scale':"",'StitchGroup':"1",'Options':"ProcessingInstructions=\"1:2\""}
    ws.addRow ( nextRow )
    nextRow = {'Run(s)':"13469",'ThetaIn':"0.7",'TransRun(s)':"13463+13464",'Qmin':"0.01",'Qmax':"0.06",'dq/q':"0.04",'Scale':"",'StitchGroup':"2",'Options':""}
    ws.addRow ( nextRow )

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "TBLFile.tbl")

    # perform the algorithm
    SaveTBL(Filename=savefile,InputWorkspace=ws)

    #the file contains
    # Run(s),ThetaIn,TransRun(s),Qmin,Qmax,dq/q,Scale,StitchGroup,Options
    # 13460,0.7,13463+13464,0.01,0.06,0.04,,1,
    # 13470,2.3,13463+13464,0.035,0.3,0.04,,0,
    # 13462,2.3,13463+13464,0.035,0.3,0.04,,1,ProcessingInstruction="1:2"
    # 13469,0.7,13463+13464,0.01,0.06,0.04,,1,
    print("File Exists: {}".format(os.path.exists(savefile)))

.. testcleanup:: ExTBLSimple

    os.remove(savefile)

Output:

.. testoutput:: ExTBLSimple

    File Exists: True

.. categories::

.. sourcelink::
