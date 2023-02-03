=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- A new interface has been created: the :ref:`Indirect Data Manipulation Interface <interface-inelastic-data-manipulation>`, from the Indirect Data Analysis interface the :ref:`Sqw <indirect-sqw>`, :ref:`Moments <indirect-moments>`, and :ref:`Symmetrise <indirect-symmetrise>` tabs have been moved into the new interface along with :ref:`Elwin <elwin>` and :ref:`Iqt <iqt>` from the Indirect Data Analysis interface.
- :ref:`LoadEMU <algm-LoadEMU>` now loads data from the beam monitor and adds missing environment parameters, reactor power, and scan period to the log when loading EMU event files.
- Added support for sparse workspace features in the :ref:`Indirect Correction Interface <interface-indirect-corrections>`.


Bugfixes
--------



Algorithms
----------

New features
############


Bugfixes
############
- A problem when running :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption-v1>` on an indirect instrument using the Sparse Instrument feature has been fixed. Detectors that are associated with an inactive analyser in the instrument definition no longer cause an error about retrieving the efixed value.

:ref:`Release 6.6.0 <v6.6.0>`