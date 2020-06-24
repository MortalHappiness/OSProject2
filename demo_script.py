import subprocess
import sys

demo_folder = ["medium_case", "sample_input_1", "sample_input_2"]
method = ["mmap", "fcntl"]
for path in demo_folder:
    cmd = ["ls", "./input/"+path]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    file_name = stdout.decode().split('\n')[:-1]
    
    if sys.argv[1] == "m":
        for i in range(len(file_name)):
            file_name[i] = "./input/" + path + '/'+file_name[i]
        file_string = ' '.join(file_name)
        for m in method:
            cmd = ["./user_program/master", str(len(file_name)), file_string, m]
            print(" ".join(cmd))
            proc = subprocess.Popen(cmd, stdout = subprocess.PIPE)
    elif sys.argv[1] == "s":
        for i in range(len(file_name)):
            file_name[i] = "./output/" + path + '/'+file_name[i]
        file_string = ' '.join(file_name)
        for m in method:
            cmd = ["./user_program/slave", str(len(file_name)), file_string, m, "3.133.134.14"]
            print(" ".join(cmd))
            proc = subprocess.Popen(cmd, stdout = subprocess.PIPE)

            cmd = ["diff", "-q", "./input/"+path, "./output/"+path]
            print(" ".join(cmd))
            proc = subprocess.Popen(cmd, stdout = subprocess.PIPE)