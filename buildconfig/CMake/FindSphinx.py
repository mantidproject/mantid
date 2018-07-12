import os.path as osp
import sphinx
print('{0};{1}'.format(sphinx.__version__, osp.dirname(sphinx.__file__)))
