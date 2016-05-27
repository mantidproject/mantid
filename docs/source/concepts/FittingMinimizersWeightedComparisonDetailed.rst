
.. _Minimizers_weighted_comparison:

Comparison of minimizers, with weighted least squares
-----------------------------------------------------

.. include:: minimizers_comparison/color_definitions.txt

Here we provide a summary and detailed results for a comparison of
minimizers in Mantid when using the cost function weighted least
squares.  For an explanation of the comparison approach see the
:ref:`General comparison of minimizers <FittingMinimizers>`. The
difference between the results presented here and the general is that
the cost function used is least squares weigted by the input
errors and the

in all cases is least squares weigted by the
input errors, named in the list of cost functions available in Mantid
as `Least squares`. This is the default cost function in Mantid.


. This cost function is named in the list of cost functions
available in Mantid as `Least squares`, it is the default cost
function in Mantid and the most commonly used for neutron data.

.. _Minimizers_weighted_comparison_in_terms_of_accuracy:

Comparison in terms of accuracy
###############################


Summary, median ranking
^^^^^^^^^^^^^^^^^^^^^^^
a

.. include:: minimizers_comparison/v3.7.0/comparison_weighted_v3.7_acc_summary.txt

Accuracy for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
b

.. include:: minimizers_comparison/v3.7.0/comparison_weighted_v3.7_acc_nist_lower.txt

Accuracy for individual NIST problems, "average" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.7.0/comparison_weighted_v3.7_acc_nist_average.txt

Accuracy for individual NIST problems, "higher" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.7.0/comparison_weighted_v3.7_acc_nist_higher.txt


.. _Minimizers_weighted_comparison_in_terms_of_run_time:

Comparison in terms of run time
###############################

Summary, median ranking
^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.7.0/comparison_weighted_v3.7_runtime_summary.txt

Run time for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.7.0/comparison_weighted_v3.7_runtime_nist_lower.txt

Run time for individual NIST problems, "average" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.7.0/comparison_weighted_v3.7_runtime_nist_average.txt

Run time for individual NIST problems, "higher" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: minimizers_comparison/v3.7.0/comparison_weighted_v3.7_runtime_nist_higher.txt
