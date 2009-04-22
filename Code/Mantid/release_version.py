vf = open('vers.txt','r')
v = vf.readlines()

f = open('Kernel/inc/MantidKernel/MantidVersion.h','w')

f.write('//This file is automatically created by Mantid/Code/Mantid/build.bat(sh)\n')
f.write('#ifndef MANTID_VERSION\n')
f.write('#define MANTID_VERSION "')
f.write(v[0].strip()+'.'+v[1].strip())
f.write('"\n#endif\n')

f.close()
