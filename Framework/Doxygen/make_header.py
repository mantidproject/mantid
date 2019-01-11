# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from six.moves import range #pylint: disable=redefined-builtin
import sys

tracking = """<script>
  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');

  ga('create', 'UA-59110517-2', 'auto');
  ga('send', 'pageview');

</script>
"""

with open(sys.argv[1], 'r') as inFile:
    content = inFile.readlines()
    inFile.close()

for i in range(len(content)):
    if r"</head>" in content[i]:
        content[i] = tracking + content[i]

with open(sys.argv[1][:-3], 'w') as outFile:
    outFile.write(''.join(content))
    outFile.close()
