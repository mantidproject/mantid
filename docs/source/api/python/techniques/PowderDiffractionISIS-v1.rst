==============================
ISIS Powder Diffraction Script
==============================

.. automodule:: Powder_ISIS
   :members:

Description
-----------
A wide variety of algorithms are available within Mantid for processing of data,
writing results files, etc.  These may be run interactively, one at a time, but
in general scripting (using the Python scripting language) enables a series of
algorithms to be called in a specific order to carry out more complex tasks, such
as data normalisation and writing data files suitable for input to Rietveld
refinement programmes (GSAS, FullProf, etc).

All the script files used to process GEM, HRPD, INS and Polaris data have been
integrated to Mantid which can be found inside the following directory on a Windows
machine: `C:\MantidInstall\scripts\PowderISIS`.


Run Requirement
---------------
There are a preference file and a small script which is required to run inside
`Scripting Window <http://docs.mantidproject.org/nightly/interfaces/ScriptingWindow.html>`_
on mantid in order to carry out the data normalisation:


The preference file, a file with an extension name of .pref (e.g: UserPrefFile.Ppref)
will contain the following at least:
 - vanadium, background and empty sample container (if used) run numbers
 - parameters  for carrying out a sample absorption correction (if required)
 - Directories which can be assigned to RAW files, Vanadium Directory
*These directories have been left blank but can be changed and set by each user*
*on their preference. The directories are also modifiable via small script which*
*is ran inside* :ref:`Scripting Window <interfaces-ScriptingWindow>`

The script which is required to be written inside `Scripting Window <http://docs.mantidproject.org/
nightly/interfaces/ScriptingWindow.html>`_ on Mantid contains:
 - Details of the location for all the files which will be utilised
 - Name of the pref file and folders
 - The run number(s) you wish to process

Folder & Files Structure
------------------------

Folder and files pre structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When ExistingV is equal to yes inside the pref file being utilised, you are required to have nexus files named after the
value of CorrVanFile inside the pref file. In this case we are using POLARIS instrument so we shall need 5
nexus files as you can see inside the calibration folder below (5 is the number of banks for Polaris).

.. image:: /images/PowderISIS_pre_structure_extV_yes.png
   :scale: 20%


Folder and files post structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Once the process has completed these are the out pull files which should be if ExistingV is equal to yes in pref file

.. image:: /images/PowderISIS_Post_structure_extV_yes.png
   :scale: 20%

Usage
-----

**Example - General Script Utilised To Process Powder Diffraction With Polaris**

.. code-block:: python

   from mantid.simpleapi import *
   from mantid import config

   import cry_ini
   import cry_focus

   # Utilises the `User Directories <http://www.mantidproject.org/SplittersWorkspace>`_ in Mantid
   # Make the directory of where your file lies, are right on top in `Manage User Directories <http://www.mantidproject.org/SplittersWorkspace>`_
   DIRS = config['datasearch.directories'].split(';')
   # Or provide a directory here instead: dirs #X:


   expt = cry_ini.Files('POLARIS', RawDir=(DIRS[0] + "POLARIS"), Analysisdir='test', forceRootDirFromScripts=False, inputInstDir=DIRS[0])
   expt.initialize('Cycle_15_2', user='Mantid_tester', prefFile='UserPrefFile_15_2.pref')
   expt.tell()

   cry_focus.focus_all(expt, "79514", Write_ExtV=False)


.. autoclass:: Powder_ISIS
   :members: