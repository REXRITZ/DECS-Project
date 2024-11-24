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
responsetime = []
throughput = []

file = open(filename, 'r')
for line in file.readlines():
    info = line.rstrip().split(' ')
    labels.append(float(info[0]))
    responsetime.append(float(info[1]))
    throughput.append(float(info[2]))


# plt.plot(labels, responsetime)
plt.plot(responsetime, throughput)


# plt.xlabel("#clients")
# plt.ylabel("Time taken (sec)")
# plt.title("#clients vs Time taken")

# plt.xlabel("#clients")
# plt.ylabel("Requests/time")
# plt.title("#clients vs requests/time")

plt.xlabel("#requests")
plt.ylabel("throughput (requests/time)")
plt.title("#requests vs requests/time")

# plt.xlim(left=0)
# plt.ylim(bottom=0)
plt.grid()
plt.savefig('results/plot.png')
