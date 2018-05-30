.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a WorkspaceGroup whose children are single-spectra workspaces
corresponding to focused banks, and saves their scattering angles
**two-theta** and **phi** (sometimes called **eta**) to a 4-column
format readable by MAUD.

The columns are, in order, **Bank number**, **Group number**,
**two-theta**, and **phi**. An example file looks something like this::

  bank :    1  group:     4    9.12160000026    0.0
  bank :    2  group:     5    8.15584000023    30.0000000009
  bank :    3  group:     6    8.03516799229    150.000000004
  bank :    4  group:     7    9.0611418429    180.000000005
  ... (and so on)


Usage
-----

.. testcode:: SaveBankScatteringAngles

   import os
  
   input_ws = Load("ENGINX_277208_focused_bank_2.nxs")
   group = GroupWorkspaces(InputWorkspaces=[input_ws])

   output_file = os.path.join(config["defaultsave.directory"], "grouping.new")

   SaveBankScatteringAngles(InputWorkspace=group, Filename=output_file)

   with open(output_file) as f:
       contents = f.read().split("\n")

   print("File contents (there is only one line as we only bothered saving one spectra):")
   print(contents[0])

.. testcleanup:: SaveBankScatteringAngles

   os.remove(output_file)
			
Output:
       
.. testoutput:: SaveBankScatteringAngles

   File contents (there is only one line as we only bothered saving one spectra):
   bank :    0  group:     1201    89.9396035211    180.0000000000
		
