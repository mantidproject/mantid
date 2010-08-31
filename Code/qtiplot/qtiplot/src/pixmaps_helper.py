"""Helper file for pixmaps.cpp"""

f = open("allxpm.txt")
f_out = open("allxpm_out.txt", 'w')
try:
  for line in f:
    line = line.replace('\n', '')
    line = line.replace('\r', '')
    f_out.write( 'else if (name == "%s") return QPixmap(%s);\n' % (line, line))
finally:
  f.close()

