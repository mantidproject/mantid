import re

suppressions_file = "CppCheck_Suppressions.txt.in"
file_and_line_regex = r"^constParameterCallback:\$\{CMAKE_SOURCE_DIR\}(.*):(\d+)$"
add_const_regex = r"^([^<>]*)\b \&(.*)$"

# with open(suppressions_file, "r") as sf:
#     for line in sf:
#         match = re.match(file_and_line_regex, line)
#         if match is not None:
#             broken_file = "../../" + match.group(1)
#             line_number = int(match.group(2))
#             with open(broken_file, "r") as f:
#                 lines = f.readlines()
#             broken_line = lines[line_number - 1]
#             line_match = re.match(add_const_regex, broken_line)
#             if line_match is None:
#                 print(f"Could not find match: {broken_line}")
#             else:
#                 fixed_line = f"{line_match.group(1)} const *{line_match.group(2)}\n"
#                 lines[line_number - 1] = fixed_line
#                 with open(broken_file, 'w') as f:
#                     f.writelines(lines)

with open(suppressions_file, "r") as sf:
    suppressions = sf.readlines()

for i, line in enumerate(suppressions):
    match = re.match(file_and_line_regex, line)
    if match is not None:
        broken_file = "../.." + match.group(1)
        line_number = int(match.group(2))
        with open(broken_file, "r") as f:
            lines = f.readlines()
        broken_line = lines[line_number - 1]
        line_match = re.match(add_const_regex, broken_line)
        if line_match is None:
            print(f"Could not find match: {broken_file}:{line_number}")
        else:
            pass
            # fixed_line = f"{line_match.group(1)} const *{line_match.group(2)}\n"
            # lines[line_number - 1] = fixed_line
            # with open(broken_file, 'w') as f:
            #     f.writelines(lines)
            # suppressions[i] = ""

# with open(suppressions_file, "w") as sf:
#     sf.writelines(suppressions)
