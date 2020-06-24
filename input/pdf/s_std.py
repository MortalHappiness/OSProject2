import os
from subprocess import check_output, call 
slave_path  = "../../user_program/slave"
method = "m"
master_ip = "127.0.0.1"
for f in os.listdir("."):
    if(".py" not in f):
        ret = check_output(slave_path + " 1 " + "tmp " +  method + " " + master_ip, shell=True) 
        print("slave " + ret)
