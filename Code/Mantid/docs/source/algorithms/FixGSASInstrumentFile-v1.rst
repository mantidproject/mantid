.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

GSAS instrument is required to have exact 80 characters each line.
FixGSASInstrument will check the input GSAS instrument file whether each
line of it will have 80 characters. If any line contains fewer than 80
characters, this algorithm will fill the line with space to 80
characters.

Usage
-----

**Example - correct a GSAS instrument file to 80 characters per line:**

.. testcode:: ExHistSimple

  # Set up default directory and file name
  import os

  defaultsavepath = config["defaultsave.directory"]
  if config.hasProperty("defaultsave.directory"):
    defaultpath = config["defaultsave.directory"]
  elif config.hasProperty("default.savedirectory"):
    defaultpath = config["default.savedirectory"]
  else:
    defaultpath = "/tmp/"

  savefile = os.path.join(defaultpath, "fixed.iparm")

  # Run the algorithm
  FixGSASInstrumentFile(InputFilename="PG3HR60_FmPython.iparm", OutputFilename=savefile)

  # Load new file and check each line
  wfile = open(savefile, "r")
  lines = wfile.readlines()
  numlinefewer80b = 0
  for line in lines:
    if len(line) > 0:
      line = line.split("\n")[0]
      if len(line) != 80:
        numlinefewer80b += 1

  # Print out result
  print "Corrected File: Number of lines that are not equal to 80 characters = ", numlinefewer80b

.. testcleanup:: ExHistSimple

  # Clean
  import os
  os.remove(savefile)


Output:

.. testoutput:: ExHistSimple

  Corrected File: Number of lines that are not equal to 80 characters =  0

.. categories::
