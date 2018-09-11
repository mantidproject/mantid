.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates an `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_
from a plain ASCII file. Uses a simple
format for the file described below. This algorithm is suitable for
importing small volumes of data only. This algorithm does not scale well
for large input workspaces. The purpose of this algorithm is to allow
users to quickly import data from existing applications for purposes of
comparison.

Format
------

The file must contain a **DIMENSIONS** section listing the
dimensionality of the workspace. Each input line is taken as a new
dimension after the **DIMENSIONS** flag, and before the **MDEVENTS**
flag. Input arguments must be separated by a space or tab. They are
provided in the order Name, ID, Units, NumberOfBins. See the Usage
examples below.

The file must contain a **MDEVENTS** flag after the **DIMENSIONS** flag.
Again, inputs are separated by a space or tab, each line represents a
unique MDEvent. There must be either NDims + 2 or NDims + 4 columns in
this section. If there are NDims + 2, columns, then *Lean* MDEvents will
be used (Signal, Error and Dimensionality only). If there are NDims + 4
columns, then *Full* MDEvents will be used (Signal, Error, RunNo,
DetectorId and Dimensionality). If you have provided NDims + 2 columns,
then each row is interpreted as follows:

::

 Signal Error {Dimensionality}

where the Dimensionality is an array of coordinates in each of the
dimensions listed in the **DIMENSIONS** section, and in that order. IF
there are NDims + 4 columns, then *Full* MDEvents will be used. Each row
is interpreted as follows:

::

 Signal Error RunNumber DetectorId {Dimensionality}

The usage example below shows demo files with both of these formats.

Comments are denoted by lines starting with **#**. There is no
multi-line comment.

Alternatives
------------

Other alternatives to importing/creating MDWorkspaces are
:ref:`ImportMDHistoWorkspace <algm-ImportMDHistoWorkspace>`,
:ref:`CreateMDHistoWorkspace <algm-CreateMDHistoWorkspace>` and
:ref:`CreateMDWorkspace <algm-CreateMDWorkspace>`

Usage
-----

.. include:: ../usagedata-note.txt

**Example - MDLeanEvents**

.. testcode:: ExMdLeanEvents

    ws = ImportMDEventWorkspace("demo_mdew_mdleanevents.txt")
    print("Number of Events = {}".format(ws.getNEvents()))
    print("Workspace type = {}".format(ws.id()))

Output:

.. testoutput:: ExMdLeanEvents

    Number of Events = 20
    Workspace type = MDEventWorkspace<MDLeanEvent,2>

**Example - MDEvents**

.. testcode:: ExMdEvents

    ws = ImportMDEventWorkspace("demo_mdew_mdevents.txt")
    print("Number of Events = {}".format(ws.getNEvents()))
    print("Workspace type = {}".format(ws.id()))

Output:

.. testoutput:: ExMdEvents

    Number of Events = 20
    Workspace type = MDEventWorkspace<MDEvent,2>

.. categories::

.. sourcelink::
