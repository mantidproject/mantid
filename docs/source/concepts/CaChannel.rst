.. _cachannel:

CaChannel
=========

.. contents:: Table of Contents
  :local:

Summary
-------

Describes how to use the `CaChannel <https://pypi.org/project/CaChannel/>`__ package to connect
to an instrument that uses the `EPICS <http://www.aps.anl.gov/epics/>`__ software control suite.

Installation
------------

The installation of the ``CaChannel`` depends on how you have installed mantid.
See https://www.mantidproject.org/installation the available methods.
Developers should use the `Conda Package` method

Full Installation
#################

If you installed mantid via an installer package then simplest way to install
CaChannel is to run the following script in the Workbench Script Window

.. code-block:: python

   """Install CaChannel with pip"""
   import subprocess as subp
   import sys

   # Run pip using the same executable as the running Python
   py_exe = sys.executable
   pip_cmd = (
     py_exe, "-m", "pip", "install", "--upgrade-strategy", "only-if-needed", "cachannel"
   )
   process = subp.run(pip_cmd, stdout=subp.PIPE, stderr=subp.STDOUT, encoding="utf-8")
   print(process.stdout)
   process.check_returncode()


Conda Package
#############

If you installed mantid via Conda then you can add CaChannel to your existing
environment:

.. code-block:: sh

  >conda activate your-environment-name
  >const install --channel paulscherrerinstitute cachannel

Usage
#####

Assuming the instrument can be contacted by a prefix ``PREFIX1:PREFIX2`` then to retrieve a process variable you would type the following Python code:

.. code-block:: python

   from CaChannel import CaChannel, CaChannelException, ca
   chan = CaChannel('PREFIX1:PREFIX2:PVNAME:VALUENAME')
   chan.setTimeout(5.0)
   chan.searchw()
   as_string=True # set to False if you want a numeric value
   print(chan.getw(ca.DBR_STRING if as_string else None))

where ``PVNAME:VALUENAME`` is the name of the PV variable and associated value.
The print statement should print the text value of the PV.
If no output is returned then the connection could not be made.
Check the IP address of the server is in the environment variable above and that the full name of the PV is correct.

Example PV names for ``INTER``:

- IN:INTER:DAE:TITLE

- IN:INTER:DAE:RUNNUMBER

- IN:INTER:CS:SB:Theta

- IN:INTER:CS:SB:S1VG
