.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Converts loaded/prepared Muon data to a data suitable for analysis.

Supports three modes:

-  PairAsymmetry - asymmetry is calculated for a given pair of groups,
   using the alpha value provided.
-  GroupAsymmetry - asymmetry between given group and Muon exponential
   decay is calculated.
-  GroupCount - **no asymmetry is calculated**, pure counts of the
   specified group are used.

For every mode, either one or two data acquisition period workspaces can
be provided. PeriodOperation determines in which way period data will be
merged at the end.

Usage
-----

**Example - Pair asymmetry for a single period:**

.. testcode:: ExPairAsymmetry

   y = [1,2,3] + [4,5,6]
   x = [1,2,3] * 2
   input = CreateWorkspace(x, y, NSpec=2)

   output = MuonCalculateAsymmetry(FirstPeriodWorkspace = input,
                                   OutputType = 'PairAsymmetry',
                                   PairFirstIndex = 1,
                                   PairSecondIndex = 0,
                                   Alpha = 0.5)

   print 'Output:', output.readY(0)

Output:

.. testoutput:: ExPairAsymmetry

   Output: [ 0.77777778  0.66666667  0.6       ]

**Example - Group asymmetry for two periods:**

.. testcode:: ExGroupAsymmetryMultiperiod

   y1 = [100,50,10]
   y2 = [150,20,1]
   x = [1,2,3]

   input1 = CreateWorkspace(x, y1)
   input2 = CreateWorkspace(x, y2)

   output = MuonCalculateAsymmetry(FirstPeriodWorkspace = input1,
                                   SecondPeriodWorkspace = input2,
                                   PeriodOperation = '-',
                                   OutputType = 'GroupAsymmetry',
                                   GroupIndex = 0)

   print 'Output:', output.readY(0)

Output:

.. testoutput:: ExGroupAsymmetryMultiperiod

   Output: [-0.28634067  0.60594273  0.26255546]

.. categories::
