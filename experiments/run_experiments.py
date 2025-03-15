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

def main(input_files, command_template, output_xml, tags):
    # Read existing results.xml if it exists
    if os.path.exists(output_xml):
        try:
          tree = ET.parse(output_xml)
          root = tree.getroot()
        except:
          root = ET.Element("results")
    else:
        root = ET.Element("results")
    
    # Add a new group for the current run
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    group_element = ET.Element("group")
    group_element.append(create_xml_element("start_time", timestamp))
    group_element.append(create_xml_element("tags", tags))
    root.append(group_element)
    
    for input_file in input_files:
        print(f"Processing file: {input_file}")
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
    
    # root.append(group_element)
    
    # Update the XML file after processing all files
    with open(output_xml, "w") as xml_file:
        xml_file.write(prettify_xml(root))

def f(output_xml, non_unique: bool, only_mem):
    # Read input files from a file
    with open('/home/lars/data/HaliVerTests/Unittests/experiments/unit_tests.txt', 'r') as file:
        names = [line.strip() for line in file.readlines()]
    
    postfix = "_mem" if only_mem else ""
    postfix = postfix + ("-non-unique" if non_unique else "")

    input_files = [f"{file}{postfix}.c" for file in names]
    command_template = "/home/lars/data/vercors/bin/vct --silicon-quiet --no-infer-heap-context-into-frame --dev-total-timeout=3600 --dev-assert-timeout 60 /home/lars/data/HaliVerTests/Unittests/build/{input_file}"  # Replace with your command template
    tags = "normal" if postfix == "" else postfix
    main(input_files, command_template, output_xml, tags)

if __name__ == "__main__":
    for i in range(5):
      timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
      f(f"results-{timestamp}.xml", False, False)
      f(f"results-{timestamp}.xml", False, True)
      f(f"results-{timestamp}.xml", True, False)
      f(f"results-{timestamp}.xml", True, True)