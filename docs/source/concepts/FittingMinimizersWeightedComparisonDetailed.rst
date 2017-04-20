
.. _Minimizers_weighted_comparison:

Comparison of minimizers, with weighted least squares
-----------------------------------------------------

.. include:: minimizers_comparison/color_definitions.txt

Here we provide a summary and detailed results for a comparison of
minimizers in Mantid when using weights in the cost function. For an
explanation of the comparison approach and the results obtained when
not using weights see the :ref:`General comparison of minimizers
<FittingMinimizers>`.  In the results presented here the
cost function used is least squares and weigths are defined from input
observational error estimates. See the :ref:`general concept page on
fitting <Fitting>` for an explanation on how the error estimates are
used in the least squares cost function.  This cost function is named
in the list of cost functions available in Mantid as "Least
squares". It is the default cost function in Mantid and the most
commonly used for neutron data.

For the Neutron data problems true observational errors are used as
weights, as commonly done in Mantid.  As the NIST and CUTEst problems
do not include measurement errors, assuming that these datasets would
represent data from a typical Mantid workspace we introduce
observational error estimates calculated as the square root of the
observations.

.. _Minimizers_weighted_comparison_in_terms_of_accuracy:

Comparison in terms of accuracy
###############################


Summary, median ranking
^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_acc_summary.txt

Accuracy for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_acc_nist_lower.txt

Accuracy for individual NIST problems, "average" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_acc_nist_average.txt

Accuracy for individual NIST problems, "higher" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_acc_nist_higher.txt

Accuracy for individual "CUTEst" problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_acc_cutest.txt

Accuracy for individual Neutron Data problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_acc_neutron_data.txt

.. _Minimizers_weighted_comparison_in_terms_of_run_time:

Comparison in terms of run time
###############################

Summary, median ranking
^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_runtime_summary.txt

Run time for individual NIST problems, "lower" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_runtime_nist_lower.txt

Run time for individual NIST problems, "average" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_runtime_nist_average.txt


Run time for individual NIST problems, "higher" difficulty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_runtime_nist_higher.txt

Run time for individual "CUTEst" problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_runtime_cutest.txt

Run time for individual Neutron Data problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. include:: minimizers_comparison/v3.8.0/comparison_weighted_v3.8_runtime_neutron_data.txt

