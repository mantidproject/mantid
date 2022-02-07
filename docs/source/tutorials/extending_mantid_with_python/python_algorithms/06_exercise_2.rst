.. _06_exercise_2:

==========
Exercise 2
==========

The aim of this exercise is to write a small Python algorithm that is able
to print to the Messages Box the first ``n`` numbers of the Fibonacci series.
The series starts with ``0, 1`` and then the next term is the sum of the
two previous terms.

The algorithm should:

#. Define a property that sets the maximum number of terms to print. Single
   letters are not considered good property names so a good name would be
   something like ``NTerms``
#. The property should have documentation that shows in the GUI describing
   what it is used for.
#. Validate that ``NTerms`` property is greater or equal to 0 when set in
   the GUI.
#. The log message for the value of each term should be at ``notice`` level
   and in the format: "Term 1 in the Fibonacci series is: 0"
#. Add a log message at the debug level that prints the value of the ``NTerms``
   property after it has been retrieved.

As an additional exercise in understanding errors:

#. On execution, check that the value of ``NTerms`` is less than or equal
   to 1000. If it is not then raise a ``RuntimeError``.

Once finished check your answer with the provided :ref:`02_emwp_sol`
