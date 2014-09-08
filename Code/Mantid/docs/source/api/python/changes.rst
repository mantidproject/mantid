.. _pythonapi-changes:

=============================
 Changes between 1.0 and 2.0
=============================

.. note::

   This page is intended for those users who have used Python in Mantid v1.x. For
   new users, see the `getting started guides <http://www.mantidproject.org/Main_Page#Getting_Started>`_.
   	
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
the new API is

.. code-block:: python

  from mantid.simpleapi import *

Changes
-------

* The *MantidFramework* module no longer exists, it has been replaced with the *mantid* package, i.e

 * *import MantidFramework* -> *import mantid*

* The *mtd* object can now only be used to access workspaces from Mantid. In the v1 API there
  were many additional functions attached to *mtd* such as *sendLogMessage*, *deleteWorkspace* and *settings*. These
  are no longer available, there replacements are:
  
  * *mtd.initialize()* has been removed and has no counterpart as it is unnecessary
  * *mtd.sendLogMessage("msg")* -> *logger.information("msg")*
  * *mtd.deleteWorkspace(ws)* -> *DeleteWorkspace(ws)*
  * *mtd.settings* -> *config*
  * *mtd.getSettings* -> *config*
  * *mtd.workspaceExists("ws")* -> *mtd.doesExist("ws")*
  * *mtd.settings.facility* -> *config.getFacility*
  * *mtd.getConfigProperty* -> *config.getString*

* *mtd[]* will now raise an *KeyError* if a workspace does not exist rather than returning *None*.

* The *isGroup* function on a workspace no longer exists. To test for a group use the Python *isinstance* function::

    ws = mtd["name"]
    from mantid.api import WorkspaceGroup # (only required as while the old API still exists.)
    if isinstance(ws, WorkspaceGroup):
      print "is group"
    else:
      print "is not a group"

* The *getSampleDetails()* function has been removed. It should be replaced with *getRun()*.

* The Axis functions *createNumericAxis*, *createTextAxis*, *createSpectraAxis* have been removed. A spectra axis can no longer be created
  from Python as it is the default workspace axis & changing it almost always results in unexpected behaviour. The functions have been
  replaced, they new ones take the same arguments, with:

  * *createNumericAxis* -> *NumericAxis.create*
  * *createTextAxis* -> *TextAxis.create*

* The *.workspace()* function on algorithm functions has been removed & they now return their outputs (see here for more details), i.e.::

    run = Load('SomeRunFile.ext')
    print run.getNumberHistograms()
    ei, mon_peak, mon_index, tzero = GetEi(run, 1,2, 12.0) # This will run GetEi and return the outputs as a tuple and the Python will unpack them for you

* The output workspace name is taken from the variable that it is assigned to::

    run = Load('SomeRunFile.ext') # loads the file into a workspace called "run"

* It is still possible provide a different workspace name and use mtd::

    run = Load(Filename='SomeRunFile.ext', OutputWorkspace="rawfile") # name in mantid will be "rawfile"
    rawfile = mtd["rawfile"]
    print run.getNumberHistograms()
    print rawfile.getNumberHistograms()

* The *qti* module no longer exists. All user scripts should simply use the *mantidplot* module which contains
  all of the *qti* functionality but adds protection against crashes from closed windows.

* There have also been changes with Python algorithm syntax. For this it will be most beneficial to read the new tutorial `here <http://www.mantidproject.org/Python_Algorithms_Documentation/>`_. 

Automatic Migration
-------------------

A script is included with the distribution that is able to translate simple scripts from the the old API to the new API. It covers the basics of the replacements mentioned 
above along with converting some algorithm calls. It will create a backup of the original script with the string *.mantidbackup* appended to it. Currently the script
does not handle

* old algorithm calls that use a return value, e.g. alg = Load('SomeRunFile.ext','runWS')
* Python algorithms.

Any script containing the above will raise an error in the migration process and restore the original script from the backup. 

An old API algorithm call that does **NOT** use a return value, such as

.. code-block:: python

   Load('SomeRunFile.ext','runWS')

which will be translated to

.. code-block:: python

   runWS = Load(Filename='SomeRunFile.ext')

along with any of the text replacements mentioned in the previous section
    
In order to run the script you will need to use the command line. On Windows: click start, run and type cmd; on OS X and Linux: open a terminal window. To run the script type::

    python [MANTIDINSTALL]/scripts/migrate1to2.py file
    
where [MANTIDINSTALL] should be replaced by the location of the mantid install:

* Windows: C:/MantidInstall (only the default, please put the actual location)
* Mac OS X: /Applications/MantidPlot.app
* Linux: /opt/Mantid

and *file* should be replaced by the path to a single script file.

