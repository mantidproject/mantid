.. _EnumeratedString:

EnumeratedString
==================

.. contents::
  :local:

Why EnumeratedString?
-----------------------

It is common for a property to be a string, the value of which must be one of several enumerated values.  Programmatically,
this is representing an ``enum`` object in C++, but based on a set of strings instead of integer-types.  What would be ideal
would be something like the following:

.. code-block:: cpp

   enum class StringPropertyOptions: std::string {option1="OptionOne", ... };

However, this is not allowed under C++.

The ``EnumeratedString`` objects allow for binding an ``enum`` or ``enum class`` to a vector of strings, allowing for much
of the same behavior.  This allows for easy-to-read ``if`` and ``switch`` statements, as well as easy conversions and assignments
with strings from the allowed set.  This further adds an additional layer of validation for string properties, in addition to the
``StringListValidator`` used in the property declaration.

How to use the EnumeratedString
---------------------------------

First include the ``EnumeratedString.h`` header file.

This is a template class, and its three template parameters are the name of an ``enum`` type, a *pointer* to static vector of
``std::string`` objects, and an optional *pointer* to a statically defined ``std::function`` for comparing ``std::string`` objects. The last
template parameter is defaulted to ``compareStrings`` which implements a case-sensitive string comparison. A predefined function for case-insensitive
string comparison, ``compareStringsCaseInsensitive``, is also provided as an option.

Below is an example.  Consider the mantid algorithm :code:`BakeCake`, which has a string property,
``CakeType``.  This algorithm only knows how to bake a few types of cakes.  The allowed types of cake the user can set for
this property are limited to "Lemon", "Bundt", and "Pound".

The ``EnumeratedString`` should be setup as follows:

.. code-block:: cpp

  #include "MantidKernel/EnumeratedString.h"

  namespace Mantid {

  namespace {
  enum class CakeTypeEnum {LEMON, BUNDT, POUND, enum_count};
  const std::vector<std::string> cakeTypeNames {"Lemon", "Bundt", "Pound"};
  // optional typedef
  typedef EnumeratedString<CakeEnumType, &cakeTypeNames> CAKETYPE;
  } // namespace

  // ...

  // initialize an object
  EnumeratedString<CakeTypeEnum, &cakeTypeNames> cake1 = CakeTypeEnum::LEMON;
  EnumeratedString<CakeTypeEnum, &cakeTypeNames> cake2 = "Lemon";

  //init from the typedef
  CAKETYPE cake3 = "Pound";

  bool sameCake = (cake1==cake2); //sameCake = true, a Lemon cake is a Lemon cake
  bool notSameCake = (cake1!=cake3); //notSameCake = true, a Lemon cake is not a Pound cake

Notice that the final element of the ``enum`` is called :code:`enum_count`.  This is mandatory.  This element indicates the
number of elements inside the ``enum``, and used for verifying compatibility with the vector of strings.  A compiler error
will be triggered if this is not included.

Further, the ``enum`` *must* have elements in order from 0 to :code:`enum_count`.  That is, you *CANNOT* set them like so:

.. code-block:: cpp

   enum class CakeTypeEnum : char {LEMON='l', BUNDT='b', POUND='p', enum_count=3}; // NOT ALLOWED


as this will break validation features inside the class.

Notice the use of the reference operator, :code:`&cakeTypeNames`, and *not* :code:`cakeTypeNames`.

In the above code, a :code:`CAKETYPE` object can be created either from a :code:`CakeTypeEnum`, or from one of the strings
in the :code:`cakeTypeNames` array (either by the literal, or by accessig it in the array), or from another :code:`CAKETYPE`
object.  The only assignment/comparison not directly possible is from :code:`CakeTypeEnum` to one of the strings.  Otherwise
free conversion and comparison from :code:`CAKETYPE`, :code:`CakeTypeEnum`, and strings from :code:`cakeTypeNames` is possible.

Example Use of EnumeratedString
---------------------------------

An example of where this might be used inside an algorithm is shown below:

.. code-block:: cpp

   #include "MantidAlgorithms/BakeCake.h"
   #include "MantidKernel/EnumeratedString.h"

   namespace Mantid {

   namespace {
   enum class CakeTypeEnum {LEMON, BUNDT, POUND, enum_count};
   const std::vector<std::string> cakeTypeNames {"Lemon", "Bundt", "Pound"};
   typedef EnumeratedString<CakeEnumType, &cakeTypeNames> CAKETYPE;
   } // namespace

   namespace Algorithms {

   void BakeCake::init() {
      // the StringListValidator is optional, but fails faster; the CAKETYPE cannot be set with string not in list
      declareProperty("CakeType", "Bundt", std::make_shared<Mantid::Kernel::StringListValidator>(cakeTypeNames),
         "Mandatory.  The kind of cake for algorithm to bake.");
   }

   void BakeCake::exec() {
      // this will assign cakeType from the string property
      CAKETYPE cakeType = getPropertyValue("CakeType");

      // logic can branch on cakeType comparing to the enum
      switch(cakeType){
      case CakeTypeEnum::LEMON:
         bakeLemonCake();
         break;
      case CakeTypeEnum::BUNDT:
         bakeBundtCake();
         break;
      case CakeTypeEnum::POUND:
         bakePoundCake();
         break;
      }

      getLemonsForCake("Bundt");
      getIngredientsForCake(cakeType);

      // other ways to compare
      if(cakeType == "Lemon"){
         g_log.information() << "Baking a lemon cake\n";
      }
      if(cakeType == CakeTypeEnum::BUNDT){
         g_log.information() << "Baking a bundt cake\n";
      }
      CAKETYPE poundCake = CakeTypeEnum::POUND;
      if(cakeType == poundCake){
         g_log.information() << "Baking a pound cake\n";
      }
   }

   void BakeCake::getLemonsForCake(CAKETYPE cakeType){
      if(cakeType == CakeTypeEnum::LEMON){
         g_log.information() << "Getting some lemons!\n";
      } else {
         g_log.information() << "I have no need for lemons.\n";
      }
   }

   void BakeCake::getIngredientsForCake(std::string cakeType){
      g_log.information() << "Retrieving ingredients for a " << cakeType << " cake!\n";
   }

   }// namespace Algorithms
   }// namespace Mantid

This will easily handle branching logic on the basis of a set number of possible string values, using an ``enum`` to base the set of strings.

In the code examples above, if you don't want to distinguish between names like "Lemon" and "LEMON", you can define your ``CAKETYPE`` as case-insensitive:

.. code-block:: cpp

  #include "MantidKernel/EnumeratedString.h"

  namespace Mantid {

  namespace {
  enum class CakeTypeEnum {LEMON, BUNDT, POUND, enum_count};
  const std::vector<std::string> cakeTypeNames {"Lemon", "Bundt", "Pound"};
  // optional typedef
  typedef EnumeratedString<CakeEnumType, &cakeTypeNames, &compareStringsCaseInsensitive> CAKETYPE;
  } // namespace

You can also provide your own string comparator like ``firstLetterComparator`` shown below:

.. code-block:: cpp

  #include "MantidKernel/EnumeratedString.h"

  namespace Mantid {

  namespace {
  std::function<bool(const std::string &, const std::string &)> firstLetterComparator =
    [](const std::string &x, const std::string &y) { return x[0] == y[0]; };
  enum class CakeTypeEnum {L, B, P, enum_count};
  const std::vector<std::string> cakeTypeFirstLetters {"L", "B", "P"};
  // optional typedef
  typedef EnumeratedString<CakeEnumType, &cakeTypeFirstLetters, &firstLetterComparator> CAKETYPE;
  } // namespace

in which case a "Lemon" cake will get the same ``enum`` value as a "Lime" cake.
