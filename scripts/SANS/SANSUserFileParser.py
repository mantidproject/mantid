#pylint: disable=invalid-name
import mantid
from mantid.kernel import Logger

class BackCommandParser(object):
    def __init__(self):
        super(BackCommandParser, self).__init__()
        self._set_parser_chain()

        # Parse results
        self._use_mean = None
        self._use_time = None
        self._detector = None
        self._run_number = None


    def _set_parser_chain(self):
        # Standard because they all have the same pattern, eg REAR/TIME/MEAN/RUN=1234
        self._first_level_keys_standard = ['REAR','MAIN', 'FRONT', 'HAB']

        # Special because they are not standard, eg MON/RUN=1234/MEAN/TIME
        self._first_level_keys_special = ['MON']

        self._uniform_key = ['TIME', 'UAMP']
        self._mean_key = ['MEAN', 'TOF']
        self._run_key = ['RUN=']

        # Evaluation chains 
        self._standard_chain = [self._first_level_keys_standard, self._uniform_key, self._mean_key, self._run_key]
        self._special_chain = [self._first_level_keys_special, self._run_key, self._uniform_key, self._mean_key]
        self._evaluation_chain = None

        # Key - method mapping
        self._set_up_method_map()


    def _set_up_method_map(self):
         self._method_map = {''.join(self._first_level_keys_standard): self._evaluate_detector,
                             ''.join(self._first_level_keys_special): self._evaluate_detector,
                             ''.join(self._uniform_key): self._evaluate_uniform,
                             ''.join(self._mean_key):self._evaluate_mean,
                             ''.join(self._run_key):self._evaluate_run}

    def _get_method(self, key_list):
        return self._method_map[''.join(key_list)]

    def _evaluate_mean(self, argument):
        '''
        Evalutes if the argument is either MEAN, TOF or something else.
        @param argument: string to investigate
        '''
        if argument == self._mean_key[0]:
            self._use_mean = True
        elif argument == self._mean_key[1]:
            self._use_mean = False
        else:
            raise RuntimeError("BackCommandParser: Cannot parse the MEAN/TOF value. "
                               "Read in " + argument +". "+
                               "Make sure it is set correctly.")

    def _evaluate_uniform(self, argument):
        '''
        Evalutes if the argument is eithe TIME, UAMP or something else.
        @param argument: string to investigate
        '''
        if argument == self._uniform_key[0]:
            self._use_time = True
        elif argument == self._uniform_key[1]:
            self._use_time = False
        else:
            raise RuntimeError("BackCommandParser: Cannot parse the TIME/UAMP value. "
                               "Read in " + argument +". "+
                               "Make sure it is set correctly.")

    def _evaluate_run(self, argument):
        '''
        Evalutes if the argument is RUN=
        @param argument: string to investigate
        '''
        if not argument.startswith(self._run_key[0]):
            raise RuntimeError("BackCommandParser: Cannot parse the RUN= value. "
                               "Read in " + argument +". "+
                               "Make sure it is set correctly.")

        # Remove the Run= part and take the rest as the run parameter. At this point we cannot
        # check if it is a valid run
        self._run_number  = argument.replace(self._run_key[0], "")

    def _evaluate_detector(self, argument):
        '''
        Evaluates which detector to use. At this point the validty of this has already been checkd, so
        we can just take it as is.
        @param argument: string to investigate
        '''
        self._detector = argument


    def can_attempt_to_parse(self, arguments):
        '''
        Check if the parameters can be parsed with this class
        @param param: a string to be parsed
        @returns true if it can be parsed
        '''
        # Convert to capital and split the string
        to_check = self._prepare_argument(arguments)

        # We expect 4 arguments
        if len(to_check) != 4:
            return False

        can_parse = False
        # Check if part of the first entry of the standard chain
        if self._is_parsable_with_standard_chain(to_check):
            self._evaluation_chain = self._standard_chain
            can_parse = True
        elif self._is_parsable_with_special_chain(to_check):
            self._evaluation_chain = self._special_chain
            can_parse = True

        return can_parse

    def _is_parsable_with_special_chain(self, argument):
        '''
        Check if the first entry corresponds to a standard chain, i.e. starting with monitor and followed by run spec.
        @param arguments: the string list containing the arguments
        '''
        if argument[0] in self._special_chain[0] and argument[1].startswith(self._special_chain[1][0]):
            return True
        else:
            return False

    def _is_parsable_with_standard_chain(self, argument):
        '''
        Check if the first entry corresponds to a standard chain, i.e. not starting with a monitor
        @param arguments: the string list containing the arguments
        '''
        if argument[0] in self._standard_chain[0]:
            return True
        else:
            return False

    def _prepare_argument(self, argumentstring):
        '''
        Takes an argument string and returns splits it into a list
        @param argumentstring: the input
        @returns a list
        '''
        # Convert to capital and split the string
        split_arguments = argumentstring.upper().split('/')
        return [element.strip() for element in split_arguments]

    def parse_and_set(self, arguments, reducer):
        '''
        Parse the values of the parameter string and set them on the reducer
        @param arguments: the string containing the arguments
        @param reducer: the reducer
        @returns an error message if something went wrong or else nothing
        '''
        # Parse the arguments.
        self._parse(arguments)

        # Now we set the arguments on the reducer
        #self._set_arguments_on_reducer(reducer)


    def _parse(self, arguments):
        '''
        Parse the arguments and store the results
        @param arguments: the string containing the arguments
        '''
        to_parse = self._prepare_argument(arguments)

        if not self.can_attempt_to_parse(arguments):
            raise RuntimeError("BackCommandParser: Cannot parse provided arguments."
                               "They are not compatible")

        index = 0
        for element in to_parse:
            key = self._evaluation_chain[index]
            evaluation_method = self._get_method(key)
            evaluation_method(element)
            index +=  1

    def _set_arguments_on_reducer(reducer):
        # Set the mean selection

        # Set the time selection

        # Set the detector selection

        # Set the run number
        pass
