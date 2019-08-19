# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidworkbench package

import six
import tokenize
import StringIO

from mantid.api import AlgorithmManager, AnalysisDataService as ADS
from workbench.projectrecovery.projectrecoverysaver import ALGS_TO_IGNORE, ALG_PROPERTIES_TO_IGNORE
from mantid import UsageService


def get_workspace_history_list(workspace):
    """
    Return a list of commands that will recover the state of a workspace.
    """
    alg_name = "GeneratePythonScript"
    alg = AlgorithmManager.createUnmanaged(alg_name, 1)
    alg.setChild(True)
    alg.setLogging(False)
    alg.initialize()
    alg.setProperty("InputWorkspace", workspace)
    alg.setProperty("IgnoreTheseAlgs", ALGS_TO_IGNORE)
    alg.setProperty("IgnoreTheseAlgProperties", ALG_PROPERTIES_TO_IGNORE)
    alg.setPropertyValue("StartTimestamp", UsageService.getStartTime().toISO8601String())
    alg.setProperty("AppendTimestamp", True)
    alg.setProperty("AppendExecCount", True)
    alg.execute()
    history = alg.getPropertyValue("ScriptText")
    return history.split('\n')[5:]  # trim the header and import


def convert_list_to_string(to_convert, add_new_line=True, fix_comments=False):
    string = ""
    for line in to_convert:
        if isinstance(line, six.string_types):
            string += line
        elif fix_comments and isinstance(line, tuple):
            string += line[0]
        else:
            # Assume it is a list and continue
            string += convert_list_to_string(line, add_new_line)
        if add_new_line:
            string += "\n"
    return string


def guarantee_unique_lines(script):
    all_lines = []
    script_string_io = StringIO.StringIO(script)
    tokens = tokenize.generate_tokens(script_string_io.readline)
    line = None
    for t in tokens:
        # Start a new line when we see a name
        if line is None and t[0] == tokenize.NAME:
            line = [t]
        # End the line when we see a logical line ending
        elif t[0] == tokenize.NEWLINE:
            # Only care about the line if it has a comment
            have_comment = any(x[0] == tokenize.COMMENT for x in line)
            if have_comment:
                # line[-1][1][1:] is the comment string, with the preceeding hash stripped off;
                # line[0][4] is the command and the comment
                all_lines.append((line[-1][4], line[-1][1][1:]))
            line = []
        # Everything in between we care about
        elif line is not None:
            line.append(t)
    unique_lines = list(set(all_lines))

    def sorting_order_workspace_history(x):
        time, _, exec_count = x[1].split()
        return time, int(exec_count)

    unique_lines.sort(key=sorting_order_workspace_history)

    return convert_list_to_string(unique_lines, add_new_line=False, fix_comments=True)


def get_all_workspace_history_from_ads():
    workspace_histories = []
    for workspace in ADS.getObjectNames():
        workspace_histories.append(get_workspace_history_list(workspace))
    script = convert_list_to_string(workspace_histories)
    return guarantee_unique_lines(script)
