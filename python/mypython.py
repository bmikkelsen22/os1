import string
import random

for i in range(0, 3):
    file = open("file" + str(i) + ".txt", "w")
    fileText = ""
    for j in range(0, 10):
        fileText += random.choice(string.ascii_lowercase)
    file.write(fileText + "\n")
    print(fileText)

numa = random.randint(1, 42)
numb = random.randint(1, 42)
print(numa)
print(numb)
print(numa * numb)