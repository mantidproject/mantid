import mantid
import os


def remove_file(test_file_path):
    if os.path.exists(test_file_path):
        os.remove(test_file_path)


def save_to_csv(content):
    test_file_path = os.path.join(mantid.config.getString('defaultsave.directory'), 'sans_batch_test_file.csv')
    remove_file(test_file_path)

    with open(test_file_path, 'w') as f:
        f.write(content)
    return test_file_path
