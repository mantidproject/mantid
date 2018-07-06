.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves the data in a workspace into a file in the ASCII 'SPE' format.

The units used for saving will match those of the input workspace, such that if you have the units Momentum Transfer ('DeltaE') then you will get a traditional SPE file, you could choose to have the units in mod Q and then it will save to an SPQ file variant.

Format of .SPE files
####################

::

    ndet ne ! number of workspaces and number of energy bins
    ### Phi Grid ! angular boundaries i.e. (ndet+1) values
    ang(0) ang(1) … ang(7)
    ang(8) ang(9) … ang(15)
    :
    :
    … ang(ndet)
    ### Energy Grid ! Energy bin boundaries i.e. (ne+1) values
    en(0) en(1) … en(7)
    en(8) en(9) … en(15)
    :
    :
    … en(ne)
    ### S(Phi,w) ! Intensities for first workspace
    s(0) s(1) … s(7)
    s(8) s(9) … s(15)
    :
    :
    … s(ndet-1)
    ### Errors ! Standard deviation for first workspace
    err(0) err(1) … err(7)
    err(8) err(9) … err(15)
    :
    :
    … err(ndet-1)
    ### S(Phi,w) ! Intensities for second workspace
    :
    :
    ### Errors ! Standard deviation for second workspace
    :
    :
    … and so on until completed ndet workspaces


IMPORTANT NOTE: in the array corresponding to the signal, the number |-1030| is to be interpreted as not-a-number. That is, the corresponding pixel is to be masked. Usually of course, a detector is masked, so that all the signal array corresponding to that workspace will contain |-1030|. The corresponding error should be zero.

.. |-1030| replace:: -10\ :sup:`30`

Example file produced by script below
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

           1       9
    ### Phi Grid
     5.000E-01 1.500E+00
    ### Energy Grid
     0.000E+00 1.000E+00 2.000E+00 3.000E+00 4.000E+00 5.000E+00 6.000E+00 7.000E+00
     8.000E+00 9.000E+00
    ### S(Phi,w)
    -1.000E+30-1.000E+30-1.000E+30-1.000E+30-1.000E+30-1.000E+30-1.000E+30-1.000E+30
    -1.000E+30
    ### Errors
     0.000E+00 0.000E+00 0.000E+00 0.000E+00 0.000E+00 0.000E+00 0.000E+00 0.000E+00
     0.000E+00


Restrictions on the input workspace
###################################

The input workspace must contain histogram data with common binning on all spectra.

Usage
-----

**Example - Save a workspace in SPE format**

.. testcode:: ExSPESimple

    #import the os path libraries for directory functions
    import os

    # create histogram workspace
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [1,1,1,1,1,1,1,1,1] # or use dataE1=[1]*9

    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "SPEFile.spe")

    # perform the algorithm
    SaveSPE(InputWorkspace=ws1,Filename=savefile)

    print("File Exists: {}".format(os.path.exists(savefile)))

.. testcleanup:: ExSPESimple

    os.remove(savefile)

Output:

.. testoutput:: ExSPESimple

    File Exists: True

.. categories::

.. sourcelink::