try: 
    import mantidplot 
    HAS_MANTIDPLOT = True 
except(ImportError, ImportWarning): 
    HAS_MANTIDPLOT = False 

try: 
    from mantidqt.widgets.codeeditor.execution import PythonCodeExecution 
    HAS_MANTID_QT = True 
except(ImportError, ImportWarning): 
    HAS_MANTID_QT = False 

import traceback


def get_indent(line):
    return len(line) - len(line.lstrip(' \t')) 

def splitCodeString(string, return_compiled=True):
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
                    yield (code_object if return_compiled else current_statement, (statement_start, line_nbr))
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

def __execute(script, globals_dict, locals_dict):
    exec(script, globals_dict, locals_dict)

def execute_script(script, progress_action):
    """
        @param script: the script to execute
        @param progress_action: a callable expresion that takes two arguments:
            - progress: a value between 0 and 1 indicating the current progress
            - state: a string indicating the current state ('running', 'success' or 'failure')
    """
    if HAS_MANTIDPLOT: 
        execFunc = lambda script: mantidplot.runPythonScript(script, async=True)
    elif HAS_MANTID_QT:
        codeExecuter = PythonCodeExecution() 
        execFunc = codeExecuter.execute_async
    else: 
        # exec doesn't keep track of globals and locals, so we have to do it manually:
        globals_dict = dict()
        locals_dict = dict()
        execFunc = lambda script: __execute(script, globals_dict, locals_dict)


    code_blocks = [code for code in splitCodeString(script, return_compiled = not HAS_MANTIDPLOT)]
    execFunc('from mantid.simpleapi import *\n')
    for i, (code, position) in enumerate(code_blocks):
        try:
            progress_action(float(i) / float(len(code_blocks)), 'running')
            execFunc(code)
        except Exception as e:
            traceback.print_exc()
            progress_action(float(i) / float(len(code_blocks)), 'failure')
            raise e
    progress_action(1.0, 'success')



