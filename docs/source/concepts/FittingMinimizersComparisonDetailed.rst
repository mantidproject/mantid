.. _FittingMinimizersComparisonDetailed:

.. include:: minimizers_comparison/color_definitions.txt

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

.. _Minimizers_unweighted_comparison_in_terms_of_accuracy:

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
favor of the minimizers that fail more often, as some of the hard
problems are excluded from the median instead of contributing to a
higher median (lower ranking).

Alternatively, see the :ref:`summary and detailed results when using
weighted least squares as cost function
<Minimizers_weighted_comparison_in_terms_of_accuracy>`.

Accuracy for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_acc_nist_lower.txt

Accuracy for individual NIST problems, "average" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_acc_nist_average.txt

Accuracy for individual NIST problems, "higher" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_acc_nist_higher.txt


Accuracy for individual "CUTEst" problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_acc_cutest.txt

Accuracy for individual Neutron Data problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_acc_neutron_data.txt


.. _Minimizers_unweighted_comparison_in_terms_of_run_time:

Comparison in terms of run time
###############################

For details on the ranking approach and the color codes see
:ref:`above <Minimizers_individual_comparison>` and :ref:`the
summarized results <FittingMinimizers>`. Note that this comparison is
approximate and was performed on a particular platform (Ubuntu) and
machine.

Alternatively, see the :ref:`summary and detailed results when using
weighted least squares as cost function
<Minimizers_weighted_comparison_in_terms_of_run_time>`.

Run time for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_runtime_nist_lower.txt

Run time for individual NIST problems, "average" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_runtime_nist_average.txt

Run time for individual NIST problems, "higher" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_runtime_nist_higher.txt

Run time for individual "CUTEst" problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_runtime_cutest.txt

Run time for individual Neutron Data problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_runtime_neutron_data.txt
