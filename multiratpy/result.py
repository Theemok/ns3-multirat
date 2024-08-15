import os

#small script to generate average results from files

directory = "results/congestion"

with open("result.csv", 'w') as f:
    pass

seperator = ""
for file in os.listdir(directory):
    average = 0.0
    filename = os.fsdecode(file)
    with open(os.path.join(directory, filename), 'r') as f:
        values = []
        for line in f:
            items = line.split(",")
            items = [float(x) for x in items]
            values.extend(items)
        average = sum(values) / len(values)
    with open ("result.csv", "a") as f:
        f.write(seperator + filename + "," + str(average))
        seperator = "\n"

            