.. _03_exercise_1:

==========
Exercise 1
==========

The aim of this exercise is to familiarise you with the use of objects
within Python.

#. Write a ``Detector`` class contains two attributes: **id** and **name**. The
   attributes should be able to be set by a call like:
   ``Detector(1, "bank1_1")``.
#. Write an Instrument class with two attributes: **name** and list of
   **detectors**. These attributes should also be able to be set by a
   call like: ``Instrument('MyInst', dets)``.
#. Add a method to the ``Instrument`` class called ``printTree`` that will
   print the **names** & **ids** of the detectors contained within it
#. Write a class called ``Component`` that stores a single **name** attribute
#. Reorganise the ``Instrument`` & ``Detector`` classes to use ``Component``
   as a super class and share its **name** attribute
#. Check the ``printTree`` method still functions correctly.

Once finished check your answer with the provided :ref:`01_emwp_sol`
