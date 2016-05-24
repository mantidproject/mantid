.. _FittingMinimizersComparisonDetailed:

.. .. these are for the color scale "goodness of fit"
.. role:: ranking-top-1
.. role:: ranking-top-2
.. role:: ranking-med-3
.. role:: ranking-low-4
.. role:: ranking-low-5


.. .. This is for the ranking/gradient colors in the table cells
.. raw:: html

         <script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.7.1/jquery.min.js"></script>
         <script>
           $(document).ready(function() {
             $('.ranking-top-1').parent().addClass('ranking-top-1-parent');
             $('.ranking-top-2').parent().addClass('ranking-top-2-parent');
             $('.ranking-med-3').parent().addClass('ranking-med-3-parent');
             $('.ranking-low-4').parent().addClass('ranking-low-4-parent');
             $('.ranking-low-5').parent().addClass('ranking-low-5-parent');
           });
         </script>
         <style>
           .ranking-top-1-parent {background-color:#fef0d9;}
           .ranking-top-2-parent {background-color:#fdcc8a;}
           .ranking-med-3-parent {background-color:#fc8d59;}
           .ranking-low-4-parent {background-color:#e34a33;}
           .ranking-low-5-parent {background-color:#b30000;}
         </style>

.. _Minimizers_individual_comparison:

Comparison of minimizers for individual test problems
-----------------------------------------------------

As in the summarized results (:ref:`FittingMinimizers`), the best
possible results with Mantid minimizers are given a top score "1"
(lowest error, or lowest run time).

The following color codes are used to distinguish different levels of
performance, note the irregular (increasing) difference between level
boundaries):

- :ranking-top-1:`best or within 10%` (ranking < 1.1)
- :ranking-top-2:`within 33% over the best` (1.1 < ranking < 1.33)
- :ranking-med-3:`within 75% over the best` (1.33 < ranking < 1.75)
- :ranking-low-4:`within 200% over the best` (1.75 < ranking < 3)
- :ranking-low-5:`over 200% of best` (ranking > 3)

The integer numbers 1 and 2 next to the problem names denote the
starting point (all NIST problems are usually tested for two different
starting points). By clicking on the names of the test problems
(leftmost column) you can see the original definition of the problem.

.. _Minimizers_comparison_in_terms_of_accuracy:

Comparison in terms of accuracy
###############################

For details on the ranking approach and the color codes see
:ref:`above <Minimizers_individual_comparison>` and :ref:`the
summarized results <FittingMinimizers>`. When interpreting these
results note that the median shown in the summarized results is
calculated excluding undefined values. Undefined accuracy values are
obtained when the minimizers fail to produce any result and are shown
in the detailed results as "nan". For the more difficult groups of
problems the proportion of undefined values is higher for some
minimizers. This can bias the statistics shown in the summary table in
favor of the minimizers that fail more often, as some of the the hard
problems are excluded from the median instead of contributing to a
higher median (lower ranking).

Accuracy for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.7.0/comparison_v3.7_acc_nist_lower.txt

Accuracy for individual NIST problems, "average" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.7.0/comparison_v3.7_acc_nist_average.txt

Accuracy for individual NIST problems, "higher" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.7.0/comparison_v3.7_acc_nist_higher.txt

.. _Minimizers_comparison_in_terms_of_run_time:


Comparison in terms of run time
###############################

For details on the ranking approach and the color codes see
:ref:`above <Minimizers_individual_comparison>` and :ref:`the
summarized results <FittingMinimizers>`.

Run time for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.7.0/comparison_v3.7_runtime_nist_lower.txt

Run time for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.7.0/comparison_v3.7_runtime_nist_average.txt

Run time for individual NIST problems, "higher" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.7.0/comparison_v3.7_runtime_nist_higher.txt
