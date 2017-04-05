from __future__ import (absolute_import, division, print_function)
import os
import nose

_avail_modules = {
    'aggregate': 'tests/aggregate_test.py',
    'convert': 'tests/convert_test.py',
    'configs': 'tests/configs_test.py',
    'data': 'tests/imgdata_test.py',
    'imgdata': 'tests/imgdata_test.py',
    'helper': 'tests/helper_test.py',
    'tools': 'tests/importer_test.py/',
    'all': 'tests/'
}


def _run_tests(args):
    try:
        args = args[1]

        if args in ['-h', '--help', '-?']:
            # just go inside the except block
            raise IndexError

    except IndexError:
        print(
            'Please specify the folder/test file to be executed, or one of the available modules: {0}'.
            format(_avail_modules.keys()))
        return

    try:
        # check if a module name was passed
        test_path = str(_avail_modules[args])
    except KeyError:
        # it wasn't an available module, maybe a full path
        test_path = args

    print("Running tests from", test_path)

    test_path = os.path.expandvars(os.path.expanduser(test_path))

    try:
        # for verbose run add [test_path, "-vv", "--collect-only"]
        return nose.run(defaultTest=test_path, argv=[test_path])
    except ImportError:
        print('Module/test not found, try passing the path to the test \
        (e.g. tests/recon/configs_test.py) or one of the available modules: {0}'
              .format(_avail_modules.keys()))
        return


if __name__ == '__main__':
    import sys
    _run_tests(sys.argv)
