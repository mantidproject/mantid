# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Helper file for pixmaps.cpp"""

#allxpm.txt is a file with each _xpm variable name on one line
f = open("allxpm.txt")
f_out = open("allxpm_out.txt", 'w')
try:
  i = 0
  for line in f:
    line = line.replace('\n', '')
    line = line.replace('\r', '')
    #Windows compiler chockes on too-deeply nested ifs
    if (i % 20):
      f_out.write( 'else if (name == "%s") return QPixmap(%s);\n' % (line, line))
    else:
      f_out.write( 'if (name == "%s") return QPixmap(%s);\n' % (line, line))
    i += 1
finally:
  f.close()

