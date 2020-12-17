.. _muon_testing_new:

============
Muon Testing
============

.. contents:: Table of Contents
   :local:

Preparation
-----------
For these tests you will be using the files; ``EMU00051341-51343.nxs`` for 
:ref:`ionic_diffusion_test`, and ``EMU00020882-20900.nxs`` for 
:ref:`superconducting_copper_test`. You can download copies of these files 
from `here <https://sourceforge.net/projects/mantid/files/Sample%20Data/SampleData-Muon.zip/download>`_.
Once downloaded make sure to add the directory the files are stored in to your 
:ref:`manage user directories <ManageUserDirectories>`.

--------------------

.. _ionic_diffusion_test:

Ionic Diffusion test
--------------------

**Time required 5 - 10 minutes**

Instructions
############

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- Change *Instrument* to **EMU**, found in the *Home* tab
- In the runs line edit at the top of the interface, enter **51341-3**
  to load some data
- Go to the **Fitting** tab
	- Check the **Simultaneous fit over** checkbox
	- Right click the empty table area; Select **Add Function**
	- Add a **Flat Background** (*Background* > *Flat Background*)

Possible Problems
#################

- If you cannot load the data remember to have downaloded the data from 
  `here <https://sourceforge.net/projects/mantid/files/Sample%20Data/SampleData-Muon.zip/download>`_
  and to set up your :ref:`manage user directories <ManageUserDirectories>`.

---------------------------

.. _superconducting_copper_test:

Superconducting Copper Test
---------------------------

Instructions
############

Possible Problems
#################