.. _pattern_matching_with_regular_expressions:

=========================================
Pattern Matching With Regular Expressions 
=========================================

-  A common file processing requirement is to match strings within the
   file to a standard form, for example a file may contain list of
   names, numbers and email addresses. A email extraction would need to
   extract only those entries that matched which look like an email
   address.

-  Regular expressions, commonly called regexes, are ideally suited for
   this task and although they can become very complex it is also
   possible to perform many tasks with some relatively simple
   expressions.

-  At their simplest, a regular expression is simply a string of
   characters and this string would then match with only that exact
   string, e.g.

.. code:: python

   string in file: 'email'
   regex: 'email' # This is a regular expression but albeit not a very
                  # useful one as only matches with one word!

-  In reality regexes are used to search for a string that "has the
   form" of the regular expression, as in the above email example. For
   this to be possible we need to define some syntax that lets us
   specify things such as 'a number is in a range', 'a letter is one of
   a set', 'a certain number of characters' etc. For this to work some
   characters are considered special and when used in conjunction with
   each other they let the user specify the correct search criteria.

-  Here we will examine a few special characters, for a complete
   reference see http://www.regular-expressions.info/reference.html or
   search for "regular expression reference" online.

Special Characters
==================

-  An asterisk ``*`` specifies that the character preceding it can
   appear zero or more times, e.g,

.. code:: 

   regex: 'a*b'
   test: 'b'         # Matches as there are no occurrences of 'a'
   test: 'ab'        # Matches as there is a single 'a'
   test: 'aaaaaaaab' # Matches as there are multiple occurrences of 'a'
   test: 'aaaabab'   # Matches as there is an occurrence of a string of
                     # a's followed by a b

-  A range of characters, or a "character class" is defined using square
   brackets ``[]``, e.g.

.. code:: 

   regex: '[a-z]'
   test: 'm' # Matches as it is a lower case letter
   test: 'M' # Fails as it is an upper case letter
   test: '4' # Fails as it is a number

-  Several ranges can be specified such that they are all checked, e.g.

.. code:: 

   regex: '[a-z,A-Z,0-9]'
   test: 'm'  # Matches!
   test: 'M'  # Matches!
   test: '4'  # Matches!
   test: 'mm' #Fails as there are two characters

-  Combining ranges and the asterisk allows us to specify any number of
   alphanumeric characters!, e.g.

.. code:: 

   regex: '[a-z,A-Z,0-9]*'
   test: 'mm'    # Matches
   test: 'a0123' # Matches

-  To specify an exact number of characters use braces ``{}``, e.g.

.. code:: 

   regex: 'a{2}'
   test: 'abab'  # Fails as there is not two consecutive a's in the string
   test: 'aaaab' # Matches

-  For more complicated regular expressions it is not obvious whether
   you have written the expression correctly so it can be useful to
   check that it matches as you expect. For such tests there are online
   tools available such as the regex tester at
   http://www.regular-expressions.info/javascriptexample.html. Simply
   type in your regex and a test string and it will tell you a match can
   be found within your string.

Regular Expressions in Python
=============================

-  Python contains a regular expression module, called ``re`` that
   allows strings to be tested against regular expressions with a few
   lines of code. Reference: http://docs.python.org/2/library/re.html

-  The ``compile`` function also takes another optional argument
   controlling the matching process, all of which are documented at the
   above location. Here we pass the ``RE.IGNORECASE`` option meaning
   that a case-insensitive match is performed.

-  Example:

.. testcode:: regex1

   import re

   def checkForMatch(checker, test):
       if checker.match(test) != None:
           print('String matches!')
       else:
           print('String does not contain a match')
   # End of function definition

   checker = re.compile('[a-z]')
   checkForMatch(checker, 'a')  
   checkForMatch(checker, '9') 

   checker = re.compile('[a-z]', re.IGNORECASE)
   checkForMatch(checker, 'a')  
   checkForMatch(checker, 'A') 

Gives the output:

.. testoutput:: regex1

    String matches!
    String does not contain a match
    String matches!
    String matches!

-  Below we provide a more complex example of using regular expressions
   and a place where they would actually be used in a practical sense.
   The scenarios concern parsing a file with multiple lines of the form

``Running 13 tests.............OK!``

where the line has to start with the word 'Running' and end with the
word 'OK!' or the test is considered a failure.

-  Regular expressions make parsing such a file a relatively simple
   matter once the regular expression is known. Here is the full
   example:

.. code:: python

   import re

   filetestsRun = 'testResults.log'
   f = open(filetestsRun,'r')
   reTestCount = re.compile("Running\\s*(\\d+)\\s*test", re.IGNORECASE)
   reCrashCount = re.compile("OK!")
   reFailCount = re.compile("Failed\\s*(\\d+)\\s*of\\s*(\\d+)\\s*tests", re.IGNORECASE)
   testCount = 0
   failCount = 0
   testsPass = True
   for line in f.readlines():
       m=reTestCount.search(line)
       if m:
           testCount += int(m.group(1))
           m=reCrashCount.search(line)
           if not m:
               failCount += 1
               testsPass = False
       m=reFailCount.match(line)
       if m:
           # Need to decrement failCount because crashCount will
           # have incremented it above
           failCount -= 1
           failCount += int(m.group(1))
           testsPass = False
      
   f.close()

   print("Tests Passed: {}".format(testsPass))
   print("Tests Failed: {}".format(failCount))
   print("Total Tests: {}".format(testCount))

-  The loop keeps track of test crashes and failures by using regular
   expressions to match the required text within each line of the file

`Category:Tested Examples <Category:Tested_Examples>`__
