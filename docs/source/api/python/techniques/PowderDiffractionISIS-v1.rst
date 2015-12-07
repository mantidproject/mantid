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
`Scripting Window <http://docs.mantidproject.org/nightly/interfaces/
ScriptingWindow.html>`_ on mantid in order to carry out the data normalisation:

The preference file, a file with an extension name of .pref (e.g: UserPrefFile.pref)
will contain the following at least:
 - vanadium, background and empty sample container (if used) run numbers
 - parameters  for carrying out a sample absorption correction (if required)
 - Directories which can be assigned to RAW files, Vanadium Directory
*These directories have been left blank but can be changed and set by each user*
*on their preference. The directories are also modifiable via small script which*
*is ran inside* `Scripting Window <http://docs.mantidproject.org/nightly/interfaces
/ScriptingWindow.html>`_

The script which is required to be written inside `Scripting Window <http://docs.
mantidproject.org/nightly/interfaces/ScriptingWindow.html>`_ on Mantid contains:
 - Details of the location for all the files which will be utilised
 - Name of the pref file and folders
 - The run number(s) you wish to process

Files & Folders
---------------

Files & Folder Pre=Structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When ExistingV is yes inside the pref file that is being utilised, you are required to
have nexus files named after the value of CorrVanFile which is also found inside the
pref file. In this case we are using POLARIS instrument so we shall need 5 nexus files
as you can see inside the calibration folder below (5 is the number of banks for Polaris).

.. image:: /images/PowderISIS_pre_structure_extV_yes.png
   :scale: 20%

.. _post-structure-PowderISIS-ref:

Files & Folder Post-Structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Once the process has completed these are the output files which should be generated if
ExistingV is equal to yes in pref file

.. image:: /images/PowderISIS_Post_structure_extV_yes.png
   :scale: 20%

Data Layout
^^^^^^^^^^^
The structure of the folder shown above in the image can be easily modified within the
script that is written inside `Scripting Window <http://docs.mantidproject.org/nightly/
interfaces/ScriptingWindow.html>`_. The script that is being used to generate above folder
structure can be found in the :ref:`usage-PowderISIS-ref` section.
If the `Analysisdir` equals empty string or `Analysisdir` is not pass as a parameter within
the python script, the `GrpOff` and `Cycle_15_2` folder will be required in the `POLARIS`
folder.

Similarly if the `Cycle_15_2` or `user` is not passed as a parameter or passed as an empty
string `("")`, then the script shall look for the files in previous folder.

The 'GrpOff' folder is always required, within the 'GrpOff' folder is a .cal file which
gives the detector grouping (i.e. the detector banks' numbers) and the Offset to be
applied to focus each detector (this is in addition to the "ideal" instrument geometry
focusing information, which Mantid determines from the Instrument Definintion File).

The run number, vanadium number and V-Empty number data is required to be provided within
the raw directory, if the vana-peaks are also required to be removed then a file named
*VanaPeaks.dat* is needed inside the raw directory.
This will enable vana peaks to be removed by interpolation in range given within the
VanaPeaks.dat file. The vanadium number and V-Empty number is set in the pref file as
shown below.

.. code-block:: python

   #----------------------------------------------------
   # Calibration Runs Numbers
   #----------------------------------------------------
   #
   # Cycle 2013/5  Beam 40mm(h) x 15mm(w)
   # Fixed collimator in sample tank (no Jaws5)
   #
   Vanadium     78338
   V-Empty      78339
   #

The output files will vary on the values provided in pref file for the following variables,
which either equal yes or no.

.. code-block:: python

   #Output
   XYE-TOF      yes
   XYE-D        yes
   GSS          yes
   Nexus        yes

The `XYE-TOF`, `XYE-D`, `GSS` and `Nexus` files along with a copy of the Grouping file are all
generated where the pref file is located, which would be inside the 'Mantid_tester' folder in
:ref:`post-structure-PowderISIS-ref`.

.. _usage-PowderISIS-ref:

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