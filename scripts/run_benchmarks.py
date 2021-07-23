import json
import subprocess
import sys

from string import Template

if __name__ == "__main__":
    cmd_template = Template("./build/apps/parsing_speed_comparison $graph $reps")

    graph_path = sys.argv[1]
    replications = sys.argv[2]

    cmd = cmd_template.substitute(graph=graph_path, reps=replications)
    proc = subprocess.run(cmd, capture_output=True, text=True, check=True, shell=True)
    output_lines = proc.stdout.splitlines()

    result = {
            "map": [],
            "array": [],
            "vector": []
    }

    for line in output_lines:
        prefix, value = line.split()

        if prefix == "UID-based:":
            result["map"].append(int(value))
        elif prefix == "Array-based:":
            result["array"].append(int(value))
        elif prefix == "Vector-based:":
            result["vector"].append(int(value))
        else:
            print("Unknown prefix, ignoring:", prefix)

    with open("result.bench", "w") as result_file:
        json.dump(result, result_file)
