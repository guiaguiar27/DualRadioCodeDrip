import os
import random
import shutil
import subprocess

# Set the paths to the required directories and files
contiki_path = "/path/to/contiki-ng"
cooja_path = os.path.join(contiki_path, "tools/cooja")
cooja_script = os.path.join(cooja_path, "cooja")
simulation_config_template = "/path/to/simulation-template.csc"  # Path to your Cooja simulation template
output_directory = "/path/to/output"

# Ensure the output directory exists
os.makedirs(output_directory, exist_ok=True)

# Function to set random node positions in the simulation configuration file
def set_random_positions(template, output_file, num_nodes=10, area_size=(100, 100)):
    with open(template, 'r') as file:
        data = file.readlines()

    # Modify the lines that set the node positions
    for i in range(num_nodes):
        x_pos = random.uniform(0, area_size[0])
        y_pos = random.uniform(0, area_size[1])
        position_line = f'    <position x="{x_pos}" y="{y_pos}" z="0.0" />\n'
        
        # Find the correct line to replace (this is just an example, you need to adjust it to your template format)
        data[i + 1] = position_line  # Assuming the positions start at line 2

    with open(output_file, 'w') as file:
        file.writelines(data)

# Run Cooja simulation
def run_simulation(config_file, log_file):
    subprocess.run([cooja_script, "-nogui=", config_file], stdout=log_file, stderr=subprocess.STDOUT)

# Main script execution
num_simulations = 10
num_nodes = 10
area_size = (100, 100)

for i in range(num_simulations):
    # Set up the configuration file with random positions
    simulation_config_file = os.path.join(output_directory, f"simulation_{i}.csc")
    set_random_positions(simulation_config_template, simulation_config_file, num_nodes, area_size)
    
    # Define the log file path
    log_file_path = os.path.join(output_directory, f"simulation_{i}.log")
    
    # Run the simulation and save the log
    with open(log_file_path, 'w') as log_file:
        run_simulation(simulation_config_file, log_file)
    
    print(f"Simulation {i + 1}/{num_simulations} completed. Log saved to {log_file_path}")
