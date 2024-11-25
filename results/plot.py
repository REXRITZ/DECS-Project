import sys
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print("Error: too few arguments!")
    print(
        "Usage: ./plot.py <results_filename>"
    )
    exit()

filename = sys.argv[1]

labels = []
totalrequests = []
throughput = []

file = open(filename, 'r')
for line in file.readlines():
    info = line.rstrip().split(' ')
    labels.append(float(info[0]))
    totalrequests.append(float(info[1]))
    throughput.append(float(info[2]))


# plt.plot(labels, responsetime)
plt.plot(totalrequests, throughput)


plt.xlabel("#requests")
plt.ylabel("throughput (requests/time)")
plt.title("#requests vs throughput")

plt.grid()
plt.savefig('results/plot_throughput.png')
plt.close()