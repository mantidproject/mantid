===========================
Changes between 1.0 and 2.0
===========================
.. default-domain:: py

.. note::

   This page is intended for those users who have used Python in Mantid v1.X. For
   new users, see the getting started guide.
   	
After feedback from the usage of Python within Mantid it was decided that 
some changes to the API would be helpful to make general usage simpler. Unfortunately,
it was not possible to make these changes without breaking backwards compatability. 

It was therefore decided that a new API would be introduced, running alongside 
the existing API for now, to cope with these changes. This page describes the high-level
changes.

Overview
--------

The new-style API is now the default in MantidPlot so if all of your scripts run here then
you do not need to worry about importing the correct modules, it has already been done
for you. If you run from a stand-alone interpreter then the quickest way to get access to
the new API is::

    from mantid.simpleapi import *

The major differences in the new API are as follows:

* Algorithm functions now return their outputs (see here for more details), i.e.::

    run = Load('SomeRunFile.ext')
    print run.getNumberHistograms()
    ei, mon_peak, mon_index, tzero = GetEi(run, 1,2, 12.0) # This will run GetEi and return the outputs as a tuple and the Python will unpack them for you

* The output workspace name is taken from the variable that it is assigned to. The above example
  would load the file into a workspace named "run". **Note:** It is still possible to use the algorithm
  functions with keyword syntax to provide a workspace name but it should not be the preferred way of 
  working, i.e.::

    Load(Filename='SomeRunFile.ext', OutputWorkspace="run")
    run = mtd["run"]
    print run.getNumberHistograms()

* The *mtd* object can now only be used to access workspaces from Mantid. In the v1 API there
  were many additional functions attached to *mtd* such as *sendLogMessage*, *deleteWorkspace* and *settings*. These
  are no longer available, there replacements are:
  
  * *mtd.sendLogMessage("msg")* -> *logger.information("msg")*
  * *mtd.deleteWorkspace(ws)* -> *DeleteWorkspace(ws)*
  * *mtd.settings* -> *config*

* *mtd* will now raise an *KeyError* if a workspace does not exist rather than returning *None*

* The *qti* module no longer exists. All user scripts should simply use the *mantidplot* module which contains
  all of the *qti* functionality but adds protection against crashes from closed windows.

* The *getSampleDetails()* function has been removed. It should be replaced with *getRun()*.
