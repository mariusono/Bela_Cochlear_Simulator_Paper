# Bela Project Setup

This document outlines the steps required to update your Bela board to a version that supports lower sample rates and to build the CochlearSimulation project using the g++ compiler.

---

## Updating Bela

1. **Download the Updated Firmware:**
   - Visit the [Bela repository](https://github.com/BelaPlatform/Bela).
   - Click on the **master** tab and select the `better-i2c-rebased-2022` branch.
   - Download the repository as a ZIP file.

2. **Run the Update Script:**
   - Unzip the downloaded file.
   - Navigate to the `scripts` folder within the unzipped directory.
   - Connect your Bela board.
   - Run the `update_bela` script (a terminal application).  
     **Note:** Do not run any other processes on the Bela board during the update.
   - The board will shut down, update, and automatically restart.

---

## Building the CochlearSimulation Project

1. **Navigate to the Scripts Folder:**
   - Open the console in the Bela IDE.
   - Use terminal commands (e.g., `cd`, `ls`) to navigate to the appropriate directory within the Bela `scripts` folder.

2. **Build the Project:**
   - Execute the following command:
     ```bash
     ./build_project.sh ../projects/CochlearSimulation --clean -m 'COMPILER=g++ CPPFLAGS=-std=c++14'
     ```
   - This command uses the g++ compiler, which handles `std::complex` numbers better than the default compiler—ensuring proper functionality of the Butterworth library.
   - Once the build process is complete, your project should be ready to run.

---

## Troubleshooting

- **Bela Connection:** Ensure your Bela board is connected during the update.
- **Directory Navigation:** Confirm that you are in the correct directory before executing the build command.
- **Running Processes:** Avoid running any other applications on the Bela board during the update process.

---

If you encounter any issues, please refer to the Bela documentation or open an issue on the GitHub repository.
