"""This script outputs a fake refl calibration file."""
numpix = 304*256+100
outfile = "../../../../../Test/AutoTestData/refl_fake.cal"
f = open(outfile, 'w')
f.write("# refl FAKE detector file, written for a test\n")
f.write("# Format: number  UDET offset  select  group\n")
for i in xrange(numpix):
#  972       8176  0.0024427  1    1
  f.write("%5d%11d%11f%3d%5d\n" % (i, i, 0.001, 1, 1+(i % 100) ) )
f.close()

