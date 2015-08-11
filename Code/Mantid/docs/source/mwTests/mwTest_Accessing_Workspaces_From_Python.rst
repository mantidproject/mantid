:orphan:

.. testsetup:: mwTest_Accessing_Workspaces_From_Python[8]

   run=Load("MAR11015")

.. testcode:: mwTest_Accessing_Workspaces_From_Python[8]

   run = ConvertUnits(run, Target='dSpacing')


.. testsetup:: mwTest_Accessing_Workspaces_From_Python[18]

   sample=Load("MAR11015")

.. testcode:: mwTest_Accessing_Workspaces_From_Python[18]

   sample = mtd['sample'] # Assumes a workspace was created called "sample"


.. testcode:: mwTest_Accessing_Workspaces_From_Python[26]

   mtd['not in mantid']
   
   #Traceback (most recent call last):
   #  File "<stdin>", line 1, in <module>
   #KeyError: "'not in mantid' does not exist."

.. testoutput:: mwTest_Accessing_Workspaces_From_Python[26]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   KeyError: "'not in mantid' does not exist."


.. testcode:: mwTest_Accessing_Workspaces_From_Python[43]

   print ws1
   #Traceback (most recent call last):
   #  File "<stdin>", line 1, in <module>
   #NameError: name 'ws1' is not defined

.. testoutput:: mwTest_Accessing_Workspaces_From_Python[43]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   NameError: name 'ws1' is not defined


.. testcode:: mwTest_Accessing_Workspaces_From_Python[55]

   print ws2
   #Traceback (most recent call last):
   #  File "<stdin>", line 1, in <module>
   #NameError: name 'ws2' is not defined

.. testoutput:: mwTest_Accessing_Workspaces_From_Python[55]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   NameError: name 'ws2' is not defined


.. testsetup:: mwTest_Accessing_Workspaces_From_Python[66]

   ws1=Load("MAR11015")
   ws2 = CloneWorkspace(ws1)

.. testcode:: mwTest_Accessing_Workspaces_From_Python[66]

   mtd.importAll()
   print ws1
   #ws1
   print ws2
   #ws2

.. testoutput:: mwTest_Accessing_Workspaces_From_Python[66]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ws1
   ws2


.. testsetup:: mwTest_Accessing_Workspaces_From_Python[84]

   Load("MAR11015", OutputWorkspace="15 meV ei")

.. testcode:: mwTest_Accessing_Workspaces_From_Python[84]

   mtd.importAll()
   # Warning: "15 meV ei" is an invalid identifier, "_15_meV_ei" has been imported instead.
   print _15_meV_ei
   #15 meV ei

.. testoutput:: mwTest_Accessing_Workspaces_From_Python[84]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ...
   15 meV ei


