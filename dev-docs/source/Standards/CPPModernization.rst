.. _CppModernization:

=================================
C++ Code Modernization Guidelines
=================================

Whenever editing code, try to follow the "Boy Scout Rule", i.e., leave code surrounding your in a better state than before.
For Mantid in particular, pay attention to the following:

- Do not pass ``shared_ptr`` unless you are passing ownership. Just pass ``const MyClass &`` or ``MyClass &`` instead.
- Replace uses of ``new`` by ``std::make_unique``, where possible.
- Split large functions/methods into smaller functions/methods. Consider using free functions since that facilitates unit testing and code reuse.
- Remove trivial or obvious comments. If a comment can be changed into a better variable name, do so.
- Where it makes sense, replace boolean or integer flags by ``enum`` or preferably ``enum class``. Even better: Avoid them entirely, e.g., by moving code into two separate free functions.
- Use ``auto`` to capture complex return types such as ``std::vector<...>`` or ``std::map<...>``.
- If you encounter unit tests that look incomplete, such as not verifying that algorithm output, either fix it (if possible in reasonable time), or add a note to https://github.com/mantidproject/mantid/issues/25461.
