from time import strftime
import shutil
import sys
import os

def usage():
    usage = """Usage: python archive_file.py file dest_dir
    
    Simple script for archiving a file. The file is archived (copied) under a 
    directory named after the current date/time under dest_dir.
  
        file - The file the archive
        dest_dir - The directory to archive the file. 
        
    E.g.
    python archive_file.py Mantid-32bit.msi \\ndw714\Mantid-Archive\Installers\32bit
    
    will archive Mantid-32bit.msi file at  
    \\ndw714\Mantid-Archive\Installers\32bit\%Y-%m-%d-%H-%M-%S\Mantid-32bit.msi
    """
    print usage

def main():
    file_path = sys.argv[1]
    if not os.path.exists(file_path):
        raise RuntimeError('Input file "%s" not found' %s file_path)
    dest_dir = sys.argv[2]
    dest_dir = os.path.join(dest_dir, strftime("%Y-%m-%d-%H-%M-%S"))
    try:
        os.mkdir(dest_dir)
    except WindowsError, exc:
        if 'already exists' in str(exc):
            # Directory exists
            pass
        else:
            raise
    shutil.copy(file_path, dest_dir)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        usage()
    else:
        main();