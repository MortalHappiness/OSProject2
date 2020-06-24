# Random generate testcase

import os
import sys
import random
import string

# ========================================

if len(sys.argv) <= 2:
    print("Usage: python gen_testcase.py [output_folder] [sizes]")
    print("Ex:")
    print("    python gen_testcase.py ./input/test/ 256 512 1024")
    exit()

output_folder = sys.argv[1]
sizes = [int(x) for x in sys.argv[2:]]

if not os.path.exists(output_folder):
    print("The output_folder does not exist!")
    exit()

strings = string.ascii_letters + string.digits + "\n"

for i in range(len(sizes)):
    filename = "testcase" + str(i)
    filename = os.path.join(output_folder, filename)
    text = ''.join([random.choice(strings) for _ in range(sizes[i])])
    with open(filename, 'w') as fout:
        fout.write(text)

