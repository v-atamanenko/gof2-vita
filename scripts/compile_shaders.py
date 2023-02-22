#!/usr/bin/env python

import glob
import subprocess
import os

sce_vp_psp2 = []
sce_fp_psp2 = []

def gettargetname(file):
    res = file.replace("cg/", "gxp/")
    res = res.replace("cg\\", "gxp\\")
    res = res.replace(".cg", ".gxp")
    return res

for file in glob.glob("cg/*.cg"):
    with open(file) as f:
        try:
            first_line = f.readline()
        except Exception as e:
            print(f"Malformed cg shader: exception while reading first line ({str(f)})\n")
        
        if "sce_fp_psp2" in first_line:
            sce_fp_psp2.append(file)
        elif "sce_vp_psp2" in first_line:
            sce_vp_psp2.append(file)
        else:
            print(f"Malformed cg shader: no profile header found ({str(f)})\n")

for shader in sce_vp_psp2:
    command = ["psp2cgc.exe", "-profile", "sce_vp_psp2", shader, "-W4", "-Wperf", "-fastprecision", "-O4", "-o", gettargetname(shader)]
    if os.name != 'nt':
        command.insert(0, "wine")
    subprocess.run(command)

for shader in sce_fp_psp2:
    command = ["psp2cgc.exe", "-profile", "sce_fp_psp2", shader, "-W4", "-Wperf", "-fastprecision", "-O4", "-o", gettargetname(shader)]
    if os.name != 'nt':
        command.insert(0, "wine")
    subprocess.run(command)
