.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves the first spectrum of a workspace in ASCII format with .mft file name extension and can be used by software like Motofit.
The file contains minimum 21 header lines, an empty line separating the columns of data described by a header line informing about the quantities which are q, refl, refl_error and eventually q_res (FWHM), if present.
It is especially useful for saving reflectometry reduction data.
A file can be loaded back into Mantid by SaveAscii, which will not have an instrument defined and `Sample Logs` are missing.
Both, point data and histograms can be saved but note that the resulting file will always contain points for q and not bin edges but instead its bin centre.
This algorithm writes data in scientific exponential notation (E-notation) of double-precision.
The header constists of 21 or more, if required and user defined, lines which are as following: Instrument:, User-local contact:, Title:, Subtitle:, Start date + time, End date + time, Theta 1 + dir + ref numbers:, Theta 2 + dir + ref numbers:, Theta 3 + dir + ref numbers:
, (foreseen potentially added angle(s),) followed by 9 user defined parameter lines, followed by potentially added user defined parameter lines, Number of file format:, Number of data points:.
The number of the file format is set randomly to a higher value (40) in order to exceed all ILL Cosmos numbers.
For the ILL instruments D17 and FIGARO, obligatory header lines will be automatically filled by using the workspaces `Sample Logs` information which can be modified as shown in the example.
An option exists to only write data with no header lines and column description.

Usage
-----

**Example - Save a workspace in MFT ASCII**

.. testcode:: SaveMFT_general_usage

    # import the os path libraries for directory functions
    import os

    # create histogram workspace
    x = range(0, 4)
    y = range(0, 3)
    e = [1]*3
    dx = [9.5]*3

    ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, Dx=dx)

    # Create an absolute path by joining the proposed filename to a directory
    # os.path.expanduser("~") used in this case returns the home directory of the current user
    file = os.path.join(os.path.expanduser("~"), "ws.mft")

    # Add Sample Log entries
    AddSampleLog(Workspace=ws, LogName='Title', LogText='MyTest', LogType='String')
    AddSampleLog(Workspace=ws, LogName='d', LogText='0.3', LogType='Number', LogUnit='mm', NumberType='Double')

    # Save with mft extension and using the option LogList: Title will be added to an required header line and d will be additionally added which increases the number of lines by 1.
    SaveMFT(InputWorkspace=ws, Filename=file, LogList=['Title', 'd'])

.. testoutput:: SaveMFT_general_usage
    if os.path.exists(file):
      myFile = open(file, 'r')
      print myFile.read()

.. testcleanup:: SaveMFT_general_usage

    # Delete file
    os.remove(file)

.. categories::

.. sourcelink::
