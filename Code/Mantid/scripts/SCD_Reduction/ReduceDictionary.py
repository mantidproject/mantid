#pylint: disable=invalid-name
#
# File: ReduceDictionary.py
#
# This module contains some top level methods for constructing a python
# dictionary with the parameter values needed by the ReduceOneSCD_Run.py
# and ReduceSCD_Parallel.pay scripts.
#
# This method will load the dictionary of parameter name, value pairs from
# the specified configuration file.  The key words None, True and False are
# converted from a string to the corresponding value.  Also, the list of
# run numbers is built up from possibly multiple lines with the key "run_nums".
# The run numbers themselves may be specified as a comma separated list of
# individual run numbers, or ranges specified with a colon separator.
#
def LoadDictionary( *filenames, **kwargs ):
  # create a dictionary to load into
    params_dictionary = kwargs.get("existing", {})
  # create a list of run numbers
    run_nums = params_dictionary.get("run_nums", [])

    file = open(filenames[0])
    for line in file:
        line = line.strip()
        line = line.rstrip()
        if (not line.startswith('#')) and len(line) > 2:
            words = line.split()
      # error check the number of values
            if len(words) < 2:
                print "Syntax Error On Line: " + line
      # set the value
            else:
                (key, value) = words[0:2]

        # fix up special values
                if value.lower() == "none":
                    value = None
                elif value.lower() == "true":
                    value = True
                elif value.lower() == "false":
                    value = False

        # set the values
                if key == "run_nums":
                    run_nums.extend(ParseRunList(value))
                else:
                    params_dictionary[key] = value

    params_dictionary["run_nums"]=run_nums

  # it isn't awesome without recursion
    if len(filenames) > 1:
        return LoadDictionary(*filenames[1:], existing=params_dictionary)
    else:
        return params_dictionary

#
# Return a list of run numbers from a string containing a comma separated
# list of individual run numbers, and/or ranges of run numbers specified
# with a colon separator.
#
def ParseRunList( run_string ):
    run_list = []
    groups = run_string.split(",")
    for group in groups:
        runs = group.split(":")
        if len(runs) == 1:
            run_list.append( runs[0] )
        else:
            first = int(runs[0])
            last  = int(runs[1])
            for run in range(first, last+1):
                run_list.append(str(run))

    return run_list

