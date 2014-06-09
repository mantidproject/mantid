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

**Example - simple rebin of a histogram workspace:**

.. testcode:: ExHistSimple

  # Load original file and check each line
  rfile = open("PG3HR60_FmPython.iparm", "r")
  lines = rfile.readlines()
  rfile.close()

  numlinefewer80 = 0
  for line in lines:
    if len(line) != 80:
      numlinefewer80 += 1

  # Run algorithm to fix the instrument file
  FixGSASInstrumentFile(InputFilename="PG3HR60_FmPython.iparm", OutputFilename="/tmp/Fixed.iparm")

  # Load new file and check each line
  wfile = open("/tmp/Fixed.iparm", "r")
  lines = wfile.readlines()
  numlinefewer80b = 0
  for line in lines:
    if len(line) > 0:
      line = line.split("\n")[0]
      if len(line) != 80:
        numlinefewer80b += 1

  # Print out result
  print "Original File: Number of lines that are not equal to 80 characters = ", numlinefewer80
  print "Corrected File: Number of lines that are not equal to 80 characters = ", numlinefewer80b

.. testcleanup:: ExHistSimple

   import os
   os.remove("/tmp/Fixed.iparm")

Output:

.. testoutput:: ExHistSimple

  Original File: Number of lines that are not equal to 80 characters =  1510
  Corrected File: Number of lines that are not equal to 80 characters =  0

.. categories::
