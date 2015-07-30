.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Converts loaded/prepared Muon data to a data suitable for analysis. Either one or two data acquisition period
workspaces may be provided (only the first one is mandatory). When both of them are supplied, the algorithm merges the counts and
then calculates the asymmetry. PeriodOperation determines in which way period data will be
merged before the asymmetry calculation.

The algorithm supports three asymmetry types or modes:

-  PairAsymmetry - asymmetry is calculated for a given pair of groups,
   using the alpha value provided. The pair to use is specified via
   PairFirstIndex and PairSecondIndex.
-  GroupAsymmetry - asymmetry between given group (specified via GroupIndex)
   and Muon exponential decay is calculated.
-  GroupCount - **no asymmetry is calculated**, pure counts of the
   specified group (via GroupIndex) are used.

Note that this algorithm is intended to be executed on input workspaces whose spectra correspond to previously grouped spectra, hence
the term 'group' is used instead of 'spectrum'.

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
   y2 = [ 50,25, 5]
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

   Output: [ 0.1524242  -0.0916425  -0.71360777]

.. categories::

.. sourcelink::
