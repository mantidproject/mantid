
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will extract the background from a dataset where sample is rotated through multiple positions, so that I don't need to make a separate background measurement.

This algorithm requires at least 2 input :ref:`EventWorkspace` that are all binned identically. The proton charge of the workspaces are check to be within 1% of each other as this algorithm assume all the data was collected in the same conditions.

This algorithm operates by first grouping the detectors of the input workspaces with the provided ``GroupingFile``.  Then iterating through each bin of each spectra of the grouped workspaces, selecting the range (defined by ``PercentMin`` and ``PercentMax`` properties) of workspaces sorted by intensity for that bin, copying the events from the associated ungrouped input to the output workspace. The output is then normalized by the number of input workspaces selected.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - GenerateGoniometerIndependentBackground**

.. testcode:: GenerateGoniometerIndependentBackgroundExample

  # create 4 sample workspaces with different intensities
  for n in range(1,5):
      ws = CreateSampleWorkspace(WorkspaceType='Event', Function='Flat background', BinWidth=10000, NumEvents=n*1000, NumBanks=2, BankPixelWidth=1, OutputWorkspace=f"w{n}")
      print(f"Input workspace w{n} intensity = ", ws.readY(0))

  # create a grouping workspace, group by bank
  CreateGroupingWorkspace(InputWorkspace='w1', GroupDetectorsBy='bank', OutputWorkspace='groups')
  SaveDetectorsGrouping("groups", "/tmp/groups.xml")

  # select only the lowest 25% intensity workspaces
  background = GenerateGoniometerIndependentBackground('w1,w2,w3,w4', GroupingFile="/tmp/groups.xml", PercentMin=0, PercentMax=25)
  print("Background intensity, lowest 25% =", background.readY(0))
  # select only the highest 25% intensity workspaces
  background = GenerateGoniometerIndependentBackground('w1,w2,w3,w4', GroupingFile="/tmp/groups.xml", PercentMin=75, PercentMax=100)
  print("Background intensity, highest 25% =", background.readY(0))
  # select only the middle 50%intensity workspaces
  background = GenerateGoniometerIndependentBackground('w1,w2,w3,w4', GroupingFile="/tmp/groups.xml", PercentMin=25, PercentMax=75)
  print("Background intensity, middle 50% =", background.readY(0))

Output:

.. testoutput:: GenerateGoniometerIndependentBackgroundExample

  Input workspace w1 intensity =  [500. 500.]
  Input workspace w2 intensity =  [1000. 1000.]
  Input workspace w3 intensity =  [1500. 1500.]
  Input workspace w4 intensity =  [2000. 2000.]
  Background intensity, lowest 25% = [500. 500.]
  Background intensity, highest 25% = [2000. 2000.]
  Background intensity, middle 50% = [1250. 1250.]

.. categories::

.. sourcelink::

