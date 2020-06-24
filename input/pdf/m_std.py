import os
from subprocess import check_output, call
import subprocess 
master_path = "../../user_program/master"
slave_path  = "../../user_program/slave"
method = "m"
master_ip = "127.0.0.1"
for fn in os.listdir("."):
    if(".py" not in fn):
        cmd = f'{master_path} 1 {fn} {method}'
        print(cmd)
        break
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        stdout, stderr = proc.communicate()
        print("master " + stdout)
