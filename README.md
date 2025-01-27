## Installation Instructions

Please refer to the installation instructions before running the code.

## Running the Code

Refer to the documentation in `docs/USAGE.md` for additional information on running the code.

def write_instructions():
    # Get the run command from main.py
    from main import generate_run_command
    run_command = generate_run_command()
    # Write the instructions to the README file
    with open('README.md', 'a') as f:
        f.write('To run the code, execute the following command: \n')
        f.write(run_command)