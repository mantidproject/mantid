.. algorithm::

.. summary::

.. alias::

.. properties::

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

Overview
--------




Usage
-----

**Example - General Script Utilised To Process Powder Diffraction**

.. testcode:: PowderDiffSimple

from mantid.simpleapi import *
from mantid import config

import cry_ini
import cry_focus

# Utilises the `User Directories <http://www.mantidproject.org/SplittersWorkspace>`_ in Mantid
# Make the directory of where your file lies, are right on top in `Manage User Directories <http://www.mantidproject.org/SplittersWorkspace>`_
DIRS = config['datasearch.directories'].split(';')

expt = cry_ini.Files('POLARIS', RawDir=(DIRS[0] + "POLARIS"), Analysisdir='test', forceRootDirFromScripts=False, inputInstDir=DIRS[0])
expt.initialize('Cycle_15_2', user='Mantid_tester', prefFile='UserPrefFile_15_2.pref')
expt.tell()

cry_focus.focus_all(expt, "79514", Write_ExtV=False)

.. testcleanup:: PowderDiffSimple

Output:

.. testoutput:: PowderDiffSimple

.. categories::

.. sourcelink::
