import xml.etree.ElementTree as ET
import subprocess
import re

def parse_xml_exp(input_xml):
    tree = ET.parse(input_xml)
    root = tree.getroot()
    experiments = {}
    experiments_mem = {}

    for group in root.findall('group'):
        i = group.find('i').text

        for file in group.findall('file'):
            name = file.find('name').text
            file_info = {
                'name': name,
                'return_code': file.find('return_code').text,
                'elapsed_time': round(float(file.find('elapsed_time').text)),
                'backend_duration': extract_backend_duration(file.find('stdout').text)
            }
            base_name = get_base_filename_exp(name)
            tags = get_tags_exp(name)
            if("mem" in name):
                res_map = experiments_mem
            else:
                res_map = experiments
            

            if base_name not in res_map:
                res_map[base_name] = {}
            if tags not in res_map[base_name]:
                res_map[base_name][tags] = []
            res_map[base_name][tags].append(file_info)

    return experiments, experiments_mem

def extract_backend_duration(stdout):
    match = re.search(r"Done: BackendVerification \(at [^,]+, duration: (\d+):(\d+):(\d+)\)", stdout)
    if match:
        hours, minutes, seconds = map(int, match.groups())
        return hours * 3600 + minutes * 60 + seconds
    return None

def get_base_filename_exp(name):
    if "_non_unique" in name:
        name = name.replace("_non_unique", "")
    if "_mem" in name:
        name = name.replace("_mem", "")
    if name.endswith(".c"):
        name = name[:-2]
    return name

def get_tags_exp(name):
    result = ""
    if "_non_unique" in name:
        result = result + "Normal"
    else:
        result = result + "Unique"
    return result

def neutral(color: bool)-> str:
    return ""

def green(color: bool)-> str:
    return "\\cellcolor{ForestGreen!25}" if color else ""

def red(color: bool)-> str:
    return "\\cellcolor{BrickRed!25}" if color else ""

def color_result_line(res_nr, n_unique, avr_t_unique, avr_v_unique, n_normal, avr_t_normal, avr_v_normal)->str:
    unique_empty = n_unique == 0
    normal_empty = n_normal == 0
    current_line = ""

    if res_nr == 0:  # Only color cells for return code 0
        current_line += f"& {green(not normal_empty and unique_empty)}{red(normal_empty)} {n_normal}"

        color_normal_v = not normal_empty and (unique_empty or avr_v_normal < avr_v_unique)
        current_line += f"& {avr_t_normal} & {neutral(color_normal_v)} {avr_v_normal}"

        current_line += f"& {green(not unique_empty and normal_empty)}{red(unique_empty)} {n_unique}"

        color_unique_v = not unique_empty and (normal_empty or avr_v_unique < avr_v_normal)
        current_line += f"& {avr_t_unique} & {neutral(color_unique_v)} {avr_v_unique}"
        
        

        if(not normal_empty and not unique_empty):
            speedup = round(avr_v_normal/avr_v_unique, 2)
            current_line += f"& {green(speedup > 1)}{red(speedup < 1)} {speedup}"
        else:
            current_line += "& "
    elif res_nr ==3:
        # For timeout we print no time
        current_line += f"& {n_unique} & - & -"
        current_line += f"& {n_normal} & - & -"
    else:
        current_line += f"& {n_unique} & {avr_t_unique} & {avr_v_unique}"
        current_line += f"& {n_normal} & {avr_t_normal} & {avr_v_normal}"

    
    return current_line

def generate_latex_tabular_exp(experiments, is_mem: bool=False):
    latex = []
    result_name = {0: "\\checkmark", 1: "$\\times$", 2: "Error", 3: "T.O."}

    latex.append("\\begin{tabular}{lll|rrr|rrr|r}")
    latex.append("\\hline")
    latex.append(" & & & \\multicolumn{3}{c|}{\\textbf{Base}} & \\multicolumn{3}{c|}{\\textbf{Unique}} & \\\\")
    latex.append("\\textbf{Name} & \\textbf{V} & \\textbf{Result} & \\textbf{\\#} & \\textbf{T$_t$} & " +
      "\\textbf{T$_v$} & \\textbf{\\#} & \\textbf{T$_t$} & \\textbf{T$_v$} & \\textbf{Speedup$_v$} \\\\")
    latex.append("\\hline")

    total_normal = 0
    total_unique = 0
    total_v_normal = 0
    total_v_unique = 0
    for base_name, tags in experiments.items():
        base_name = base_name.replace("_", "\_")
        if(base_name[-1] in ["0", "1", "2", "3"]):
            version = base_name[-1]
            base_name = base_name[:-2]
        else:
            version = ""
        def get_avr(xs):
            return round(sum(xs)/len(xs)) if xs else ""
        
        unique_runs = tags["Unique"]
        normal_runs = tags["Normal"]
        prev_base_name = ""
        tag_printed = False
        rest_conv_printed = True
        if version == "":
          if(base_name == "depthwise\_separable\_conv"):
            first_line = "\\multicolumn{2}{l}{depthwise\_}"
            second_line = "\\multicolumn{2}{l}{separable\_conv}"
          else:
            first_line = f"\\multicolumn{{2}}{{l}}{{{base_name}}}"
        elif version == "0":
          first_line = f"{base_name} & {version}"
        else:
          first_line = f" & {version}"
        for i in range(0, 4):
            if(base_name == "depthwise\_separable\_conv"):
                if not tag_printed:
                    current_line = first_line
                    rest_conv_printed = False
                if tag_printed and not rest_conv_printed:
                    current_line = second_line
            else:
                current_line = first_line if not tag_printed else "& "
            current_line += f"& {result_name[i]}"
            times_t_unique = [run.get('elapsed_time') for run in unique_runs if run.get('return_code') == str(i)]
            times_t_normal = [run.get('elapsed_time') for run in normal_runs if run.get('return_code') == str(i)]
            times_v_unique = [run.get('backend_duration') for run in unique_runs if run.get('return_code') == str(i)]
            times_v_normal = [run.get('backend_duration') for run in normal_runs if run.get('return_code') == str(i)]
            
            avr_t_unique = get_avr(times_t_unique)
            avr_t_normal = get_avr(times_t_normal)
            avr_v_unique = get_avr(times_v_unique)
            avr_v_normal = get_avr(times_v_normal)
            current_line += color_result_line(i, len(times_t_unique), avr_t_unique, 
                                                  avr_v_unique, len(times_t_normal), avr_t_normal, avr_v_normal)
            if i == 0 and len(times_t_unique)>0 and len(times_t_normal)>0:
                total_normal += avr_t_normal
                total_v_normal += avr_v_normal
                total_unique += avr_t_unique
                total_v_unique += avr_v_unique

            if len(times_t_unique) > 0 or len(times_t_normal) > 0:
                latex.append(current_line + " \\\\")
                if tag_printed:
                    rest_conv_printed = True
                tag_printed = True
            elif i == 3 and not rest_conv_printed:
                latex.append("separable\_conv & & & & & & & & \\\\")
        # if(prev_base_name != base_name[:-2] and (version =="" or version == "3")):
        if(prev_base_name != base_name[:-2]):
            latex.append("\\hline")
        prev_base_name = base_name[:-2]
    speedup = round(total_v_normal/total_v_unique, 2)
    latex.append("\\hline")
    latex.append(f"Total & & \\checkmark & & {total_normal} & {total_v_normal} & & {total_unique} & {total_v_unique} & {green(speedup > 1)}{red(speedup < 1)} {speedup}  \\\\")
    latex.append("\\hline")
    latex.append("\\end{tabular}")
    print(f"Total normal: {total_normal}, total unique: {total_unique}, mem:{is_mem}")
    return latex

def parse_xml(input_xml):
    tree = ET.parse(input_xml)
    root = tree.getroot()
    experiments = {}

    for group in root.findall('group'):
        i = group.find('i').text

        for file in group.findall('file'):
            name = file.find('name').text
            file_info = {
                'name': name,
                'return_code': file.find('return_code').text,
                'elapsed_time': round(float(file.find('elapsed_time').text)),
                'backend_duration': extract_backend_duration(file.find('stdout').text)
            }
            base_name = get_base_filename(name)
            tags = get_tags(name)
            if base_name not in experiments:
                experiments[base_name] = {}
            if tags not in experiments[base_name]:
                experiments[base_name][tags] = []
            experiments[base_name][tags].append(file_info)

    return experiments

def get_base_filename(name):
    if "_non_unique" in name:
        name = name.replace("_non_unique", "")
    if "CB" in name:
        name = name.replace("CB", "")
    if name.endswith(".c"):
        name = name[:-2]
    return name

def get_tags(name):
    result = ""
    if "_non_unique" in name:
        result = result + "Normal"
    else:
        result = result + "Unique"
    if "CB" in name:
        result = result + "-CB"
    else:
        result = result + "-NCB"
    return result

def generate_latex_padre(experiments):
    names = {"StepHalide": "\\texttt{step}",
              "SubDirectionHalide": "\\texttt{sub\\_direction}",
                "SolveDirectionHalide": "\\texttt{solve\\_direction}",
                  "PerformIterationHalide": "\\texttt{perform\\_iteration}"}
    latex = []
    i = 0
    result_name = {0: "\\checkmark", 1: "$\\times$", 2: "Error", 3: "T.O."}
    latex.append("\\newcommand{\widthPadre}{0.7}")
    for base_name, tags in experiments.items():
        latex.append("\\subfloat[\\label{tab:" + base_name + "}\\texttt{" + names[base_name] +"}]{")
        latex.append("\\resizebox{\\widthPadre\\textwidth}{!}{")
        latex.append("\\begin{tabular}{ll|rrr|rrr|r}")
        latex.append("\\hline")
        latex.append(" & & \\multicolumn{3}{c|}{\\textbf{Base}} & \\multicolumn{3}{c}{\\textbf{Unique}} & \\\\")
        latex.append("\\textbf{Version} & \\textbf{Result} & \\textbf{\\#} & \\textbf{T$_t$} & \\textbf{T$_v$}"+ 
                     " & \\textbf{\\#} & \\textbf{T$_t$} & \\textbf{T$_v$} & \\textbf{Speedup$_v$} \\\\")
        latex.append("\\hline")

        def get_avr(xs):
            return round(sum(xs)/len(xs)) if xs else ""
        
        for tag in ["CB", "NCB"]:
            unique_runs = tags["Unique-" + tag]
            normal_runs = tags["Normal-" + tag]
            tag_printed = False
            for i in range(0, 4):
                current_line = f"{tag}" if not tag_printed else ""
                current_line += f"& {result_name[i]}"
                times_t_unique = [run.get('elapsed_time') for run in unique_runs if run.get('return_code') == str(i)]
                times_t_normal = [run.get('elapsed_time') for run in normal_runs if run.get('return_code') == str(i)]
                times_v_unique = [run.get('backend_duration') for run in unique_runs if run.get('return_code') == str(i)]
                times_v_normal = [run.get('backend_duration') for run in normal_runs if run.get('return_code') == str(i)]
                
                avr_t_unique = get_avr(times_t_unique)
                avr_t_normal = get_avr(times_t_normal)
                avr_v_unique = get_avr(times_v_unique)
                avr_v_normal = get_avr(times_v_normal)
                
                current_line += color_result_line(i, len(times_t_unique), avr_t_unique, 
                                                  avr_v_unique, len(times_t_normal), avr_t_normal, avr_v_normal)

                if len(times_t_unique) > 0 or len(times_t_normal) > 0:
                    latex.append(current_line + " \\\\")
                    tag_printed = True
                
            latex.append("\\hline")
        latex.append("\\end{tabular}")
        latex.append("}")
        latex.append("}")
        latex.append("\\\\")
    
    return "\n".join(latex)


def generate_latex_main():
    latex = []
    latex.append("\\documentclass{article}")
    latex.append("\\usepackage{amssymb}")
    latex.append("\\usepackage{booktabs}")
    latex.append("\\usepackage{subcaption}")
    latex.append("\\usepackage{colortbl}")
    latex.append("\\usepackage[dvipsnames]{xcolor}")
    latex.append("\\usepackage{graphicx}")
    latex.append("\\captionsetup[subfigure]{position=bottom}")
    latex.append("\\begin{document}")
    latex.append("\\newcommand{\\haliver}{HaliVer}")

    latex.append("\\begin{table}[t]")
    latex.append("\\centering")
    latex.append("\\caption{\\label{tab:chp6-results-exp-mem}Verification results for the experiments of HaliVer from Chapter 4.}")
    latex.append("\\input{results/exp-mem.tex}")
    latex.append("\\end{table}")

    latex.append("\\begin{table}[t]")
    latex.append("\\centering")
    latex.append("\\caption{\\label{tab:chp6-results-exp}Verification results for the experiments of HaliVer from Chapter 4.}")
    latex.append("\\input{results/exp.tex}")
    latex.append("\\end{table}")

    latex.append("\\begin{table}[t]")
    latex.append("\\centering")
    latex.append("\\caption{\\label{tab:chp6-results}Verification results for \\texttt{step}, \\texttt{sub\\_direction}, \\texttt{solve\\_direction}, and \\texttt{perform\\_iteration} produced by \\haliver. We use abbreviations for versions with concrete bounds (\\textbf{CB}), nonconcrete bounds (\\textbf{NCB}), \\textbf{unique} and const type qualifiers, and no type qualifiers (\\textbf{Normal}).}")
    latex.append("\\input{results/padre.tex}")
    latex.append("\\end{table}")
    latex.append("\\end{document}")
    return "\n".join(latex)

def main(input_xml_exp, input_xml_padre, output_tex):
    experiments, experiments_mem  = parse_xml_exp(input_xml_exp)
    latex_exp = generate_latex_tabular_exp(experiments_mem, True)
    with open("results/exp-mem.tex", 'w') as f:
        f.write("\n".join(latex_exp))

    latex_exp = generate_latex_tabular_exp(experiments, False)
    with open("results/exp.tex", 'w') as f:
        f.write("\n".join(latex_exp))

    experiments_padre = parse_xml(input_xml_padre)
    latex_padre= generate_latex_padre(experiments_padre)
    with open("results/padre.tex", 'w') as f:
        f.write(latex_padre)

    latex_main = generate_latex_main()
    with open(output_tex, 'w') as f:
        f.write(latex_main)

    # Generate PDF using pdflatex
    subprocess.run(['pdflatex', '-output-directory', 'results', output_tex])
    out_base = output_tex.replace(".tex", "")
    subprocess.run(['rm', f"{out_base}.aux", f"{out_base}.log"])

if __name__ == "__main__":
    input_xml_exp = "results/exp-2025-03-18.xml" 
    input_xml_padre = "results/padre-2025-03-18.xml"
    output_tex = "results/table.tex"
    main(input_xml_exp, input_xml_padre, output_tex)