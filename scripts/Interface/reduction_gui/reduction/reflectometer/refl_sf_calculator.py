#pylint: disable=invalid-name
"""
    This class holds all the necessary information to create a reduction script.
    This is a fake version of the Reducer for testing purposes.
"""
import time
from reduction_gui.reduction.scripter import BaseReductionScripter
import sys

# Check whether Mantid is available
try:
    import mantidplot
    HAS_MANTID = True
except:
    HAS_MANTID = False

class REFLSFCalculatorScripter(BaseReductionScripter):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """

    def __init__(self, name="REFL"):
        super(REFLSFCalculatorScripter, self).__init__(name=name)

    def create_script(self, script_part2):
        """
        This creates the special script for the sfCalculator algorithm
        """
        algo = 'sfCalculator.calculate'

        script_split = script_part2.split('\n')
        new_script = ''

        run_number = []
        attenuator = []

        peak_from = []
        peak_to = []
        back_from = []
        back_to = []

        tof_range = [0.0,200000.0]
        incident_medium = ''
        incident_medium_index = -1

        scaling_factor_file = ''

#        print 'in create_script'
#        print script_split

        for _line in script_split:
            if _line != '':
                _line_split = _line.split(':')
                _arg = _line_split[0]
                _val = _line_split[1]

                if _arg == 'Scaling factor file':
                    if scaling_factor_file == '':
                        scaling_factor_file = _val.strip()

                if _arg == 'Run number':
                    run_number.append(_val)
                    continue

                if _arg == 'TOF from':
                    if tof_range[0] == 0.0:
                        tof_range[0] = float(_val)
                    continue

                if _arg == 'TOF to':
                    if tof_range[1] == 200000.0:
                        tof_range[1] = float(_val)
                    continue

                if _arg == 'Incident medium':

                    _val=_val[4:-3]
                    if incident_medium.strip() == '':
                        incident_medium = _val
                    continue

                if _arg == 'Incident medium index':
                    if incident_medium_index == -1:
                        incident_medium_index = int(_val)

                if _arg == 'Number of attenuator':
                    attenuator.append(_val)
                    continue

                if _arg == 'Peak from pixel':
                    peak_from.append(_val)
                    continue

                if _arg == 'Peak to pixel':
                    peak_to.append(_val)
                    continue

                if _arg == 'Back from pixel':
                    back_from.append(_val)
                    continue

                if _arg == 'Back to pixel':
                    back_to.append(_val)
                    continue

        run_attenuator = []
        for (run,att) in zip(run_number, attenuator):
            run_attenuator.append(run.strip() + ':' + att.strip())
        join_string = ','
        script_run_attenuator = join_string.join(run_attenuator)

        list_peak_back = []
        for (_peak_from, _peak_to, _back_from, _back_to) in zip(peak_from, peak_to, back_from, back_to):
            list_peak_back.append([int(_peak_from),int(_peak_to),int(_back_from),int(_back_to)])

        new_script = algo + '(string_runs="' + script_run_attenuator + '"'
        new_script += ',list_peak_back=' + str(list_peak_back)

        #retrieve right incident medium

        incident_medium_list = incident_medium.split(',')
        incident_medium = incident_medium_list[incident_medium_index]
        new_script += ',incident_medium="' + incident_medium.strip() + '"'

        new_script += ',output_file_name="' + scaling_factor_file + '"'

        new_script += ',tof_range=' + str(tof_range) + ')'

        if scaling_factor_file == '':
            return ''

        return new_script

    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# %s scaling factor calculation script\n" % self.instrument_name
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())

        script += "import os\n"
        script += "import mantid\n"
        script += "from mantid.simpleapi import *\n"
        script += "import sfCalculator\n"

        script += "REF_RED_OUTPUT_MESSAGE = ''\n\n"

        script_part2 = ''
        for item in self._observers:
            if item.state() is not None:
                script_part2 += str(item.state())

        _script = self.create_script(script_part2)
        if _script == '':
            print 'Please define a Scaling Factor File Name'
            raise RuntimeError

        script += _script

        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()

        return script

    def apply(self):
        """
            Apply the reduction process
        """
        if HAS_MANTID:
            script = self.to_script(None)

            print script

            try:
                t0 = time.time()
                exec script
                delta_t = time.time()-t0
                print REF_RED_OUTPUT_MESSAGE
                print "SF calculation time: %5.2g sec" % delta_t
                # Update scripter
                for item in self._observers:
                    if item.state() is not None:
                        item.state().update()
            except:
                # Update scripter [Duplicated code because we can't use 'finally' on python 2.4]
                for item in self._observers:
                    if item.state() is not None:
                        # Things might be broken, so update what we can
                        try:
                            item.state().update()
                        except:
                            pass
                raise RuntimeError, sys.exc_value
        else:
            raise RuntimeError, "SF calculation could not be executed: Mantid could not be imported"


