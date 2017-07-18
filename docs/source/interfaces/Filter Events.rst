Filter Events Interface
=======================

.. contents:: Table of Contents
  :local:
   
Description
-----------
 
The Filter Events interface provides aid to users in order to set up event filter with different 
:ref:`functionalities-GenerateEventFilter-ref`. This interface utilises the algorithm 
:ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` which is able to generate event splitters
according to user’s requirement for filtering events. 
The generated time splitters are stored either in a SplittersWorkspace or a 
:ref:`MatrixWorkspace <MatrixWorkspace>`. Both of them will be used by algorithm 
:ref:`FilterEvents <algm-FilterEvents>` which filters the events from a EventWorkspace to one or 
multiple EventWorkspaces according to an input :ref:`SplittersWorkspace`
containing a series of splitters. 

On the bottom section the user has an option of :ref:`filterbylogv-GenerateEventFilter-ref`  
or :ref:`filterbytime-GenerateEventFilter-ref`. User also has an option of filtering events using 
:ref:`advsetup-FilterEventUI-ref`. 


Layout
------

.. figure:: /images/FilterEventsGUI.png
   :align: center
   :width: 400


Data Loading
------------

For the data loading step, MantidPlot provides an interface in which the user shall select the 
appropriate file via *Browse* button or by entering an integer as a valid run number in the 
*File/Run* field, if the instrument name is set up in Mantid. The user must provide a file, 
which contains valid sample logs 
for splitting and :ref:`EventWorkspace <EventWorkspace>`. The data will be loaded once the user 
has clicked upon the *Load* button. 
With the help of *drop-down* list, user is be able to load more than one file and then click 
on the *Use* button which will load the selected
file. If the file cannot be found in the *drop-down* list then user can click `Refresh` button 
which should update 
the *drop-down* list and enable user to select any browsed file.

Plotting
--------

Once the file has been selected then the Interface would automatically generate the plot using 
the provided file. If the user wish to plot another file then that could be achieved by selecting 
another file from *drop-down* list and clicking on the *Use* button, which will automatically 
plot the new graph generated from the selected file. 


Output & Storing Event Splitters
--------------------------------

The output will be one or multiple workspaces according to the number of index in splitters. The 
output workspace name is the combination of parameter OutputWorkspaceBaseName and the index in 
splitter. The *Splitter Title* field is used as title of output splitters workspace and information 
workspace. An event splitter used in Mantid contains start time, stop time and target workspace. Any 
data structure that has the above 3 properties can serve as an event splitter. There are two types of 
output workspaces for storing event splitters that are supported by 
:ref:`GenerateEventsFilter <algm-GenerateEventsFilter>` algorithm.

- :ref:`SplittersWorkspace`: It is a
  :ref:`TableWorkspace <Table Workspaces>` that has 3 columns for start time, 
  stop time and target workspace for events within start time and stop time. This type of workspace is 
  appropriate for the case that the amount of generated event splitters are not huge.


- :ref:`MatrixWorkspace <MatrixWorkspace>`: It uses X-axis to store time stamp in total nanoseconds 
  and Y-axis to store target workspace. For example, :math:`[x_i, x_i+1]` and :math:`[y_i]` construct 
  an event filter as start time is :math:`[x_i]`, stop time is :math:`[x_i+1]`, and target workspace 
  is :math:`[y_i-th]` workspace. If :math:`[y_i]`, is less than 0, then it means 
  that all events between time :math:`[x_i]` and :math:`[x_i+1]` will be discarded. This type of 
  workspace is appropriate for the case that the amount of generated event splitters are huge, because 
  processing a :ref:`MatrixWorkspace <MatrixWorkspace>` is way faster than a 
  :ref:`TableWorkspace <Table Workspaces>` in Mantid.

.. _advsetup-FilterEventUI-ref:

Advanced Setup
--------------

The **Advanced Setup** section on the bottom of the interface provides access to the following 
functionality:

+------------------+----------------------------------------------------------+
|Action            | Effect                                                   |
+==================+==========================================================+
| TOF Correction   | Type of correction on neutron events to sample time from |
| To Sample        | detector time.                                           |
+------------------+----------------------------------------------------------+
| Fast Log         | Fast log will make output workspace to be a maxtrix      |
|                  | workspace.                                               |
+------------------+----------------------------------------------------------+
| Generate Filter  | Use multiple cores to generate events filter by log      |
| In Parallel      | values. Default as Serial which uses a single core and   | 
|                  | Parallel uses multiple cores.                            |		
+------------------+----------------------------------------------------------+
| Spectrum without | Approach to deal with spectrum without detectors.        |
| Detector         |                                                          |
+------------------+----------------------------------------------------------+
| Filter By Pulse  | Filter the event by its pulse time only for slow sample  |
| Time             | environment log.This option can make execution of        |
|                  | algorithm faster. But it lowers precision.               |
+------------------+----------------------------------------------------------+
| Output Workspace | If selected, the minimum output workspace is indexed     |
| Indexed From 1   | from 1 and continuous.                                   |
+------------------+----------------------------------------------------------+
| Group Output     | Option to group all the output workspaces.               |
| Workspace        |                                                          |
+------------------+----------------------------------------------------------+
| Split Sample     | If selected, all sample logs will be splitted by the     |
| Log              | event splitters. It is not recommended for fast event    |
|                  | log splitters.                                           |
+------------------+----------------------------------------------------------+


.. categories:: Interfaces FilterEventUI
