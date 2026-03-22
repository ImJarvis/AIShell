import json
import os

def generate_header():
    json_path = r'd:\Projects\AIHollowShell\src\CommandDatabase.json'
    header_path = r'd:\Projects\AIHollowShell\tests\CommandData.h'
    
    with open(json_path, 'r') as f:
        commands = json.load(f)
    
    def escape_cpp(s):
        return s.replace('\\', '\\\\').replace('"', '\\"')
    
    with open(header_path, 'w') as f:
        f.write('#pragma once\n')
        f.write('#include <vector>\n')
        f.write('#include <string>\n\n')
        f.write('struct TestCaseData {\n')
        f.write('    int id;\n')
        f.write('    std::string type;\n')
        f.write('    std::string command;\n')
        f.write('    std::string intent;\n')
        f.write('    std::string expected;\n')
        f.write('};\n\n')
        f.write('static const std::vector<TestCaseData> c_CommandDatabase = {\n')
        
        for cmd in commands:
            f.write(f'    {{ {cmd["id"]}, "{escape_cpp(cmd["type"])}", "{escape_cpp(cmd["command"])}", "{escape_cpp(cmd["intent"])}", "{escape_cpp(cmd["expected"])}" }},\n')
            
        f.write('};\n')

if __name__ == "__main__":
    generate_header()
