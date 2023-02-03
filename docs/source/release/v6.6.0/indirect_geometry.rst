=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- A new interface has been created: the Indirect Data Manipulation Interface, from the Indirect Data Analysis interface the Sqw, Moments, and Symmetrise tabs have been moved into the new interface along with Elwin and Iqt from the Indirect Data Analysis interface.
- Loads data from the beam monitor and  adds missing environment parameters, reactor power and scan period to the log when loading EMU event files using :ref:`LoadEMU <algm-LoadEMU>`.
- Add support for sparse workspace features in the indirect correction interface


Bugfixes
--------



Algorithms
----------

New features
############


Bugfixes
############
- a problem has been fixed running :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption-v1>` on an indirect instrument using the Sparse Instrument feature. Detectors that are associated with an inactive analyser in the instrument definition no longer cause an error about retrieving the efixed value

:ref:`Release 6.6.0 <v6.6.0>`