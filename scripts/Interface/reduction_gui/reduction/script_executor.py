
def get_indent(line):
    return len(line) - len(line.lstrip(' \t')) 

def splitCodeString(string):
    import code
    lines = string.splitlines(False)

    statement_start = 0
    line_nbr = 0
    current_statement = ''
    current_indent = 0
    try:
        for line in lines:
            line_nbr += 1

            if get_indent(line) <= current_indent:
                code_object = code.compile_command(current_statement, filename="<input>", symbol="single")
                if code_object is not None:
                    yield (code_object, (statement_start, line_nbr))
                    statement_start = line_nbr
                    current_statement = '\n' * line_nbr

            current_statement += line + '\n'
            continue

    except SyntaxError as e:
        e.lineno += statement_start
        print(str(e))
        print(e.text + ' ' * (e.offset-1) + '^')
        print('~~~~~~~~~~~~~~~~~~~~~~~~')
        print(current_statement)
        raise e
    except OverflowError as e:
        raise e
    except ValueError as e:
        raise e


def execute_script(script, progress_action):
    code_blocks = [code for code in splitCodeString(script)]
    globals_dict = dict()
    locals_dict = dict()
    exec('from mantid.simpleapi import *\n', globals_dict, locals_dict)
    for i, (code, position) in enumerate(code_blocks):
        progress_action(float(i) / float(len(code_blocks)))
        exec(code, globals_dict, locals_dict)
    progress_action(1.0)



