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

Migration (first cut)
---------------------

This release includes a script that is able to translate simple scripts from the the old API to the new API. It covers the basics of the replacements mentioned 
above along with converting some algorithm calls. It will create a backup of the original script with the string *.mantidbackup* appended to it. Currently the script
does not handle

* old algorithm calls that use a return value, e.g. alg = Load('SomeRunFile.ext','runWS')
* Python algorithms.

Any script containing the above will raise an error in the migration process and restore the original script from the backup. 

An old API algorithm call that does *NOT* use a return value, such as::

    Load('SomeRunFile.ext','runWS')

which will be translated to::

    runWS = Load(Filename='SomeRunFile.ext')
    
along with any of the text replacements mentioned in the previous section
    
In order to run the script you will need to use the command line. On Windows: click start, run and type cmd; on OS X and Linux: open a terminal window. To run the script type

    python [MANTIDINSTALL]/scripts/migrate1to2.py file
    
where [MANTIDINSTALL] should be replaced by the location of the mantid install:

* Windows: C:/MantidInstall (only the default, please put the actual location)
* Mac OS X: /Applications/MantidPlot.app
* Linux: /opt/Mantid

and *file* should be replaced by the path to a single script file.

