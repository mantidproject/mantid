Filter Events
=============

.. contents:: Table of Contents
  :local:
  
Description
-----------
The Filter Events interface provide all filter events related algorithms to be utilised in a single user-friendly 
interface. The algorithm filters events from a EventWorkspace to one or multiple EventWorkspaces according to an 
input SplittersWorkspace containing a series of splitters. 
On the bottom section the user has an option of applying FilterByLogValue or FilterByTime algorithm. User also has 
an option of filtering events using Advanced Setup. 

.. figure:: ../images/FilterEventsUI.png
   :align: right
   :width: 800

Data Loading
------------
For the data loading step, MantidPlot provides users with an interface, in which the user shall select the 
appropriate file via `Browse` button in the `File/Run` field. The user must provide a file, which has valid sample logs 
for splitting and contains EventsWorkspace. The data shall be loaded once the user has clicked upon the load button. 
Following that user can click on the refresh button which will display the selected file in the combo box. With the help 
of combo-box list user shall be able to load more than one file and then click on the `Use` button which will load the 
file. If the file cannot be found in the combo-box list then user can click on the `Refresh` button which should update 
the combo-box list.

Plotting
--------
Once the file has been selected then the Interface would automatically generate the plot using the provided file. The 
user may also change the imported file using combo-box and by clicking on the Use button which will plot the new graph 
generated from the selected file. 

Output
------
The output will be one or multiple workspaces according to the number of index in splitters. The output workspace name is 
the combination of parameter OutputWorkspaceBaseName and the index in splitter.

.. categories::

.. sourcelink::
