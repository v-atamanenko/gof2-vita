import glob
import subprocess
import os
import hashlib

sce_vp_psp2 = []
sce_fp_psp2 = []

def gettargetname(file):
    res = file.replace("cg/", "gxp/")
    res = res.replace("cg\\", "gxp\\")
    return res

for file in glob.glob("cg/*.frag"):
    sce_fp_psp2.append(file)

for file in glob.glob("cg/*.vert"):
    sce_vp_psp2.append(file)

for shader in sce_vp_psp2:
    sha1 = hashlib.sha1()
    shader_glsl = shader.replace("cg/", "glsl/")
    shader_glsl = shader_glsl.replace("cg\\", "glsl\\")
    with open(shader_glsl, 'rb') as f:
        while True:
            data = f.read(65536)
            if not data:
                break
            sha1.update(data)
    print("sha1 for " + shader_glsl + "is " + sha1.hexdigest().upper())
    subprocess.run(["psp2cgc.exe", "-profile", "sce_vp_psp2", shader, "-W4", "-Wperf", "-fastprecision", "-O4", "-o", os.path.join("gxp", sha1.hexdigest().upper() + ".gxp")])

for shader in sce_fp_psp2:
    sha1 = hashlib.sha1()
    shader_glsl = shader.replace("cg/", "glsl/")
    shader_glsl = shader_glsl.replace("cg\\", "glsl\\")
    with open(shader_glsl, 'rb') as f:
        while True:
            data = f.read(65536)
            if not data:
                break
            sha1.update(data)
    print("sha1 for " + shader_glsl + "is " + sha1.hexdigest().upper())
    subprocess.run(["psp2cgc.exe", "-profile", "sce_fp_psp2", shader, "-W4", "-Wperf", "-fastprecision", "-O4", "-o", os.path.join("gxp", sha1.hexdigest().upper() + ".gxp")])
