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

   # Banks 1 to 4 of a previous texture focus in isis_powder
   # We don't use the full 160 banks as the test becomes too slow
   input_group = Load("GEM61785_D_texture_banks_1_to_4.nxs")

   output_file = os.path.join(config["defaultsave.directory"], "grouping.new")
   SaveBankScatteringAngles(InputWorkspace=input_group, Filename=output_file)

   with open(output_file) as f:
       contents = f.read().rstrip().split("\n")

   print("File contents:")
   for line in contents:
       print(line)

.. testcleanup:: SaveBankScatteringAngles

   os.remove(output_file)
			
Output:
       
.. testoutput:: SaveBankScatteringAngles

    File contents:
    bank :    0  group:     4    9.1216000000    0.0000000000
    bank :    1  group:     5    8.1558400000    30.0000000000
    bank :    2  group:     6    8.0351679921    150.0000000000
    bank :    3  group:     7    9.0611418426    180.0000000000
