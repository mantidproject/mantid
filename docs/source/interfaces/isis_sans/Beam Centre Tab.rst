.. _ISIS_SANS_Beam_Centre_Tab-ref:

Beam centre tab
---------------

.. contents:: Table of Contents
  :local:

.. image::  /images/ISISSansInterface/beam_centre_page.png
   :align: right
   :width: 800px

The beam centre tab allows the position of the beam centre to be set either
manually by the user or by running the beam centre finder.

+--------------------------+-----------------------------------------------------------------------------------------+
| **Centre Position LAB**  | The centre position of the low angle bank. The first coordinate is horizontal           |
|                          | and the second vertical. These boxes are populated by the user file and the values here |
|                          | are used by the reduction.                                                              |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Centre Position HAB**  | The centre position of the high angle bank. The first coordinate is horizontal          |
|                          | and the second vertical. These boxes are populated by the user file and the values here |
|                          | are used by the reduction.                                                              |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Minimum radius limit** | The minimum radius of the region used to ascertain centre position.                     |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Maximum radius limit** | The maximum radius of the region used to ascertain centre position.                     |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Minimum Q limit**      | The minimum Q of the region used to ascertain centre position.                          |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Maximum Q limit**      | The maximum Q of the region used to ascertain centre position.                          |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Max iterations**       | The maximum number of iterations the algorithm will perform before concluding its       |
|                          | search.                                                                                 |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Tolerance**            | If the centre position moves by less than this in an iteration the algorithm will       |
|                          | conclude its search.                                                                    |
+--------------------------+-----------------------------------------------------------------------------------------+
| **General Options**      | The **Verbose** option will store the output workspaces from all iterations in memory.  |
|                          | The **Initial COM** option will if checked use a centre of mass estimate as the starting|
|                          | point of the search rather than the user input value.                                   |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Left/Right**           | Controls whether the beam centre finder searches for the centre in the                  |
|                          | left/right and up/down directions.                                                      |
+--------------------------+-----------------------------------------------------------------------------------------+
| **Run**                  | Runs the beam centre finder the boxes **1** and **2** are updated with new              |
|                          | values upon completion.                                                                 |
+--------------------------+-----------------------------------------------------------------------------------------+
