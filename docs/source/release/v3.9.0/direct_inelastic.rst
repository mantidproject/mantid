========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Improvements
------------

- New algorithm :ref:`CalculateCountRate <algm-CalculateCountRate>` allows to calculate instrument counting rate as function of the experiment 
  time to be able to filter spurions, which may sometimes appear on ISIS instruments. It can also be used to evaluate changes
  of sample reflectivity as function of some slow changing experiment's parameter e.g. temperature, magnetic field or pressure.


`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_

New features
------------

Algorithms
##########

- A utility algorithm :ref:`WorkflowAlgorithmRunner <algm-WorkflowAlgorithmRunner>` has been added to manage the running of certain data reduction workflows at ILL.

Crystal Field
-------------

- The peak widths can be fixed to or varied around values obtained from experimental or calculated instrument resolution function.
