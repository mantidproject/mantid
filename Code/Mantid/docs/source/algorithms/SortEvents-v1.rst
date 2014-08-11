.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

In an :ref:`EventWorkspace <EventWorkspace>`, event binning is performed on
the fly. The algorithm for binning requires a list of events sorted by
time of flight, so it will perform a sort (once) on each pixel -
however, this is done on request and without using multiple CPUs). To
speed up the calculation, the Sort algorithm pre-sorts by Time of
Flight, using multiple CPUs. Using this algorithm is completely
optional.


Usage
-----

**Example - Sort events by X value:**

.. testcode:: ExSortByX

  LoadNexusProcessed(Filename=r'testeventsort.nxs',OutputWorkspace='TestEventWS',LoadHistory='0')
  SortEvents(InputWorkspace='TestEventWS', SortBy="X Value")

  ws = mtd["TestEventWS"]
  ev1 = ws.getEventList(1)
  ptimes = ev1.getPulseTimes()
  tofs = ev1.getTofs()
  for eindex in xrange(10):
    print "Spectrum 1: Event %d, Pulse Time = %s, TOF = %d" % (eindex, str(ptimes[eindex]), tofs[eindex])

.. testcleanup:: ExSortByX

  DeleteWorkspace(Workspace=ws)

Output:

.. testoutput:: ExSortByX

  Spectrum 1: Event 0, Pulse Time = 1990-01-01T00:00:01.547472179 , TOF = 28522466
  Spectrum 1: Event 1, Pulse Time = 1990-01-01T00:00:01.943767507 , TOF = 31878622
  Spectrum 1: Event 2, Pulse Time = 1990-01-01T00:00:01.074349417 , TOF = 68579583
  Spectrum 1: Event 3, Pulse Time = 1990-01-01T00:00:02.147299004 , TOF = 137022165
  Spectrum 1: Event 4, Pulse Time = 1990-01-01T00:00:00.459551021 , TOF = 155513220
  Spectrum 1: Event 5, Pulse Time = 1990-01-01T00:00:01.404601841 , TOF = 189530310
  Spectrum 1: Event 6, Pulse Time = 1990-01-01T00:00:00.137946636 , TOF = 224973368
  Spectrum 1: Event 7, Pulse Time = 1990-01-01T00:00:02.029599992 , TOF = 271677101
  Spectrum 1: Event 8, Pulse Time = 1990-01-01T00:00:00.972665472 , TOF = 302753430
  Spectrum 1: Event 9, Pulse Time = 1990-01-01T00:00:01.831699602 , TOF = 319443029

**Example - Sort events by Pulse time and TOF:**

.. testcode:: ExSortByPulseTOF

  LoadNexusProcessed(Filename=r'testeventsort.nxs',OutputWorkspace='TestEventWS',LoadHistory='0')
  SortEvents(InputWorkspace='TestEventWS',SortBy='Pulse Time + TOF')

  ws = mtd["TestEventWS"]
  ev1 = ws.getEventList(1)
  ptimes = ev1.getPulseTimes()
  tofs = ev1.getTofs()
  for eindex in xrange(10):
    print "Spectrum 1: Event %d, Pulse Time = %s, TOF = %d" % (eindex, str(ptimes[eindex]), tofs[eindex])


.. testcleanup:: ExSortByPulseTOF

  DeleteWorkspace(Workspace=ws)

Output:

.. testoutput:: ExSortByPulseTOF

  Spectrum 1: Event 0, Pulse Time = 1990-01-01T00:00:00.006085261 , TOF = 1384183147
  Spectrum 1: Event 1, Pulse Time = 1990-01-01T00:00:00.015057807 , TOF = 366664899
  Spectrum 1: Event 2, Pulse Time = 1990-01-01T00:00:00.060799751 , TOF = 1569052921
  Spectrum 1: Event 3, Pulse Time = 1990-01-01T00:00:00.096752392 , TOF = 1501703890
  Spectrum 1: Event 4, Pulse Time = 1990-01-01T00:00:00.097102049 , TOF = 2056073477
  Spectrum 1: Event 5, Pulse Time = 1990-01-01T00:00:00.098120140 , TOF = 1780138738
  Spectrum 1: Event 6, Pulse Time = 1990-01-01T00:00:00.137946636 , TOF = 224973368
  Spectrum 1: Event 7, Pulse Time = 1990-01-01T00:00:00.145045684 , TOF = 1887014465
  Spectrum 1: Event 8, Pulse Time = 1990-01-01T00:00:00.194277849 , TOF = 1608271930
  Spectrum 1: Event 9, Pulse Time = 1990-01-01T00:00:00.213847028 , TOF = 1778837570


.. categories::





