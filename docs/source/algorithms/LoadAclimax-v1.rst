.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
LoadAclimax loads csv output aCLIMAX files. Only dynamical structure factor which corresponds to 1, 2, 3, 4-10
quantum events is loaded. Workspaces which includes 'origin_quantum_event_4' in its name stores overtones 4-10 calculated by
aCLIMAX (origin_quantum_event_1-4). In case PhononWings are ticked on also workspaces for phonon wings are created (phonon_wing1-4).
Apart from partial workspaces also total workspace is created which is a sum of all partial workspaces.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - loading crystal benzene csv data :**

.. testcode:: AbinsCastepSimple

    benzene_wrk = LoadAclimax(aClimaxCVSFile="benzene.csv")


    for name in benzene_wrk.getNames():
        print name

Output:

.. testoutput:: AbinsCastepSimple

   benzene_wrk_quantum_event_1
   benzene_wrk_quantum_event_2
   benzene_wrk_quantum_event_3
   benzene_wrk_quantum_event_4
   benzene_wrk_total