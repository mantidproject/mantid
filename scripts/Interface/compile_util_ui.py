import os


def compile_ui(ui_filename):
    """
    Hand this a ui file and it will make a py file.
    """
    pyuic_dir = os.path.dirname(ui_filename)
    pyuic_filename = "ui_%s.py" % os.path.basename(ui_filename).split('.')[0]
    command = "pyuic4 -o %s/%s %s" % (pyuic_dir, pyuic_filename, ui_filename)
    os.system(command)
