======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:

Installation
------------

Windows
#######

OS X
####

User Interface
--------------

Instrument View
###############

-  The instrument view is now exposed to python as a stand-alone widget.
   In it's current implementation, the instrument view depends on the
   global workspace list in order to load data from workspaces. Example script:

.. code-block:: python

   from threading import Thread
   import time
   import sys
   import PyQt4
   import mantid.simpleapi as simpleapi
   import mantidqtpython as mpy

   ConfigService = simpleapi.ConfigService
   def startFakeEventDAE():
       # This will generate 2000 events roughly every 20ms, so about 50,000 events/sec
       # They will be randomly shared across the 100 spectra
       # and have a time of flight between 10,000 and 20,000
       try:
           simpleapi.FakeISISEventDAE(NPeriods=1,NSpectra=100,Rate=20,NEvents=1000)
        except RuntimeError:
           pass

   def captureLive():
       ConfigService.setFacility("TEST_LIVE")

       # start a Live data listener updating every second, that rebins the data
       # and replaces the results each time with those of the last second.
       simpleapi.StartLiveData(Instrument='ISIS_Event', OutputWorkspace='wsOut', UpdateEvery=0.5,
                               ProcessingAlgorithm='Rebin', ProcessingProperties='Params=10000,1000,20000;PreserveEvents=1',
                               AccumulationMethod='Add', PreserveEvents=True)


    #--------------------------------------------------------------------------------------------------
    InstrumentWidget = mpy.MantidQt.MantidWidgets.InstrumentWidget
    app = PyQt4.QtGui.QApplication(sys.argv)

    eventThread = Thread(target = startFakeEventDAE)
    eventThread.start()

    while not eventThread.is_alive():
        time.sleep(2) # give it a small amount of time to get ready

    facility = ConfigService.getFacility()
    try:
        captureLive()

        iw = InstrumentWidget("wsOut")
        iw.show()
        app.exec_()
    finally:
        # put back the facility
        ConfigService.setFacility(facility)

Plotting Improvements
#####################

Algorithm Toolbox
#################

Scripting Window
################

Documentation
#############

- Documentation has been added for fitting functions :ref:`BSpline <func-BSpline>` and
  :ref:`CubicSpline <func-CubicSpline>` then attempts to be more verbose about their use and how to
  implement them. The Documentation now contains example images of splines
  and also concrete equations that describe them
  `#15064 <https://github.com/mantidproject/mantid/pull/15064>`_

Bugs Resolved
-------------

-  VSI: Fix Mantid crash when pressing :ref:`Scale <algm-Scale>` or Cut when "builtin" node
   is selected in Pipeline Browser

SliceViewer Improvements
------------------------

-  The SliceViewer is now able to display ellipsoidal peak shapes. Note
   that the displayed ellipse is the result of the viewing plane cutting
   the peak ellipsoid.

.. figure::  ../../images/Elliptical_peaks_slice_viewer.png
   :align: center

|

Full list of
`GUI <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+GUI%22>`_
and
`Documentation <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.7%22+is%3Amerged+label%3A%22Component%3A+Documentation%22>`_
changes on GitHub
