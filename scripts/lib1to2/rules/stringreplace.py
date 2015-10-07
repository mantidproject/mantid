"""Defines the rules that simple string replacements that can be applied
to the whole text
"""
import rules
import re

__STRINGREPLACEMENTS__ = [
    (re.compile("from MantidFramework import \*"), "from mantid import *"),
    (re.compile("mtd\.initiali(s|z)e\(\)"), ""),
    (re.compile("mtd\.initiali(s|z)e\(False\)"), ""),
    (re.compile("mtd\.initiali(s|z)e\(True\)"), ""),
    (re.compile("import MantidFramework"), "import mantid"),
    (re.compile("from mantidsimple import \*"), "from mantid.simpleapi import *"),
    (re.compile("from mantidsimple import"), "from mantid.simpleapi import"),
    (re.compile("import mantidsimple as"), "import mantid.simpleapi as"),
    (re.compile("import mantidsimple"), "import mantid.simpleapi as simpleapi"),
    (re.compile("mantidsimple\."), "simpleapi."),
    (re.compile("(MantidFramework\.|)(mtd|mantid)\.initiali[z,s]e\(\)"), ""),
    (re.compile("MantidFramework\."), "mantid."),
    (re.compile("\.getSampleDetails"), ".getRun"),
    (re.compile("mtd\.settings\.facility\("), "config.getFacility("),
    (re.compile("mtd\.settings"), "config"),
    (re.compile("mtd\.getConfigProperty"), "config.getString"),
    (re.compile("mtd\.workspaceExists"), "mtd.doesExist"),
    (re.compile("(mtd|mantid).sendErrorMessage"), "logger.error"),
    (re.compile("(mtd|mantid).sendWarningMessage"), "logger.warning"),
    (re.compile("(mtd|mantid).sendLogMessage"), "logger.notice"),
    (re.compile("(mtd|mantid).sendInformationMessage"), "logger.information"),
    (re.compile("(mtd|mantid).sendDebugMessage"), "logger.debug"),
    (re.compile("(mtd|mantid).deleteWorkspace"), "mtd.remove"),
    (re.compile("\.workspace\(\)"), "")
]

class SimpleStringReplace(rules.Rules):
    """
        Implements any rules that are simply a matter of matching a pattern
        and replacing it with a fixed string
    """
    _rules = None

    def __init__(self):
        rules.Rules.__init__(self)
        self._rules = __STRINGREPLACEMENTS__

    def apply(self, text):
        """
        Returns a replacement string for the input text
        with the simple string relpacements definined by the regexes
            @param text An input string to match
            @returns A string containing the replacement or the original text
                     if no replacement was needed
        """
        for pattern, replacement in self._rules:
            text = pattern.sub(replacement, text)
        return text
