.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

For fast fequency sample logs, the time-of-flight of each neutron is
recorded at detector. As the sample log time is recorded at sample, each
neutron's flight time must be corrected to sample to be filtered
correctly by log value.

Usage
-----

**Example - generate correction table workspace and file for a powder diffractomer:**

.. testcode:: ExHistSimple

  # Load nexus file
  LoadNexusProcessed(Filename="PG3_2538_2k.nxs", OutputWorkspace="PG3_2538")

  # Calculate the correction factor
  corrws = CreateLogTimeCorrection(InputWorkspace="PG3_2538", OutputFilename="/tmp/tempcorr.txt")

  # Load output file
  cfile = open("/tmp/tempcorr.txt", "r")
  lines = cfile.readlines()
  cfile.close()

  # Print out partial result
  print("From output TableWorkspace:")
  print("detector (ID: {}) correction = {:.6f}".format(corrws.cell(0, 0), corrws.cell(0, 1)))
  print("detector (ID: {}) correction = {:.6f}".format(corrws.cell(10, 0), corrws.cell(10, 1)))
  print("detector (ID: {}) correction = {:.6f}".format(corrws.cell(100, 0), corrws.cell(100, 1)))
  print("detector (ID: {}) correction = {:.6f}".format(corrws.cell(1000, 0), corrws.cell(1000, 1)))
  print("detector (ID: {}) correction = {:.6f}".format(corrws.cell(10000, 0), corrws.cell(1000, 1)))

  print("\nFrom output file:")
  terms = lines[0].split()
  print("detector (ID: {}) correction = {}".format(terms[0], terms[1]))
  terms = lines[10].split()
  print("detector (ID: {}) correction = {}".format(terms[0], terms[1]))
  terms = lines[100].split()
  print("detector (ID: {}) correction = {}".format(terms[0], terms[1]))
  terms = lines[1000].split()
  print("detector (ID: {}) correction = {}".format(terms[0], terms[1]))
  terms = lines[10000].split()
  print("detector (ID: {}) correction = {}".format(terms[0], terms[1]))

.. testcleanup:: ExHistSimple

   DeleteWorkspace(corrws)
   import os
   os.remove("/tmp/tempcorr.txt")

Output:

.. testoutput:: ExHistSimple

  From output TableWorkspace:
  detector (ID: 27500) correction = 0.959075
  detector (ID: 27510) correction = 0.959216
  detector (ID: 27600) correction = 0.959075
  detector (ID: 28500) correction = 0.956743
  detector (ID: 102798) correction = 0.956743

  From output file:
  detector (ID: 27500) correction = 0.95908
  detector (ID: 27510) correction = 0.95922
  detector (ID: 27600) correction = 0.95907
  detector (ID: 28500) correction = 0.95674
  detector (ID: 102798) correction = 0.95036

.. categories::

.. sourcelink::
