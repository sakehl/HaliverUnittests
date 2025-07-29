import subprocess
import time
from lxml import etree as ET
from datetime import datetime
import os

def run_command(command):
    start_time = time.time()
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    stdout, stderr = process.communicate()
    end_time = time.time()
    elapsed_time = end_time - start_time
    return process.returncode, elapsed_time, stdout.decode(), stderr.decode()

def create_xml_element(tag, text):
    element = ET.Element(tag)
    element.text = text
    return element

def prettify_xml(element):
    rough_string = ET.tostring(element, pretty_print=True, encoding='UTF-8').decode('UTF-8')
    # Add XSLT reference
    xslt_ref = '<?xml-stylesheet type="text/xsl" href="style.xsl"?>\n'
    return xslt_ref + rough_string

def parse_existing_results(output_xml):
    existing_results = {}
    if os.path.exists(output_xml):
        try:
            tree = ET.parse(output_xml)
            root = tree.getroot()
            for group in root.findall('group'):
                i = group.find('i').text
                for file in group.findall('file'):
                    input_file = file.find('name').text
                    result = {
                        'return_code': file.find('return_code').text,
                        'elapsed_time': file.find('elapsed_time').text,
                        'stdout': file.find('stdout').text,
                        'stderr': file.find('stderr').text
                    }
                    existing_results[(i, input_file)] = result
        except Exception as e:
            print(f"Error parsing existing results: {e}")
    return existing_results

def main(input_files, i, command_template, output_xml, tags):
    existing_results = parse_existing_results(output_xml)
    
    # Read existing results.xml if it exists
    if os.path.exists(output_xml):
        # try:
        tree = ET.parse(output_xml)
        root = tree.getroot()
        # except:
        #     root = ET.Element("results")
    else:
        root = ET.Element("results")
    
    # Check if the group already exists
    group_element = None
    for group in root.findall('group'):
        if group.find('i').text == str(i) and group.find('tags').text == tags:
            group_element = group
            break
    
    # Add a new group for the current run if it does not exist
    if group_element is None:
        group_element = ET.Element("group")
        group_element.append(create_xml_element("i", f"{i}"))
        group_element.append(create_xml_element("tags", tags))
        root.append(group_element)
    
    for input_file in input_files:
        print(f"Processing file: {input_file}, i={i}")
        if (str(i), input_file) in existing_results:
            result = existing_results[(str(i), input_file)]
            print(f"  Skipping for i={i}. Original result: {result['return_code']}")
        else:
            command = command_template.format(input_file=input_file)
            return_code, elapsed_time, stdout, stderr = run_command(command)
            file_element = ET.Element("file")
            file_element.append(create_xml_element("name", input_file))
            file_element.append(create_xml_element("return_code", str(return_code)))
            file_element.append(create_xml_element("elapsed_time", str(elapsed_time)))
            file_element.append(create_xml_element("stdout", stdout))
            file_element.append(create_xml_element("stderr", stderr))
            print(f"  Return code was: {return_code}")
            group_element.append(file_element)
        
        # Update the XML file after each file is processed
        with open(output_xml, "w") as xml_file:
            xml_file.write(prettify_xml(root))
    
    # Update the XML file after processing all files
    with open(output_xml, "w") as xml_file:
        xml_file.write(prettify_xml(root))

def experiments(output_xml, i, non_unique=False, mem=False):
    # Read input files from a file
    with open('/home/lars/data/HaliVerTests/Unittests/experiments/experiments.txt', 'r') as file:
        names = [line.strip() for line in file.readlines()]
    
    postfix = ("_mem" if mem else "")
    postfix = postfix + ("_non_unique" if non_unique else "")

    input_files = [f"{file}_{v}{postfix}.c" for file in names for v in range(0,4)]

    if(mem):
        with open('/home/lars/data/HaliVerTests/Unittests/experiments/experiments_mem.txt', 'r') as file:
            mem_names = [line.strip() for line in file.readlines()]
        input_files = input_files + [f"{file}{postfix}.c" for file in mem_names]
 
    command_template = "/home/lars/data/vercors/bin/vct --silicon-quiet --no-infer-heap-context-into-frame --dev-total-timeout=3600 --dev-assert-timeout 60 /home/lars/data/HaliVerTests/Unittests/build/{input_file}"
    tags = "normal" if postfix == "" else postfix
    main(input_files, i, command_template, output_xml, tags)

def padre(output_xml, i, non_unique=False, cb=False):
    names = ["StepHalide", "SubDirectionHalide", "SolveDirectionHalide", "PerformIterationHalide"]
    postfix = ("CB" if cb else "")
    postfix = postfix + ("_non_unique" if non_unique else "")

    input_files = [f"{file}{postfix}.c" for file in names]
    command_template = "/home/lars/data/vercors/bin/vct --silicon-quiet --no-infer-heap-context-into-frame --dev-total-timeout=3600 --dev-assert-timeout 60 /home/lars/data/HaliVerTests/Unittests/build/{input_file}"
    tags = "normal" if postfix == "" else postfix
    main(input_files, i, command_template, output_xml, tags)

if __name__ == "__main__":
    for i in range(5):
        timestamp = "2025-03-18"
        file = f"results/padre-{timestamp}.xml"
        padre(file, i)
        padre(file, i, non_unique=True)
        padre(file, i, cb=True)
        padre(file, i, cb=True, non_unique=True)

        file = f"results/exp-{timestamp}.xml"
        experiments(file, i)
        experiments(file, i, non_unique=True)

        experiments(file, i, mem=True)
        experiments(file, i, non_unique=True, mem=True)