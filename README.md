# XIA DAQ GUI online

This project is a customized version of [PKUXIADAQ](https://github.com/wuhongyi/PKUXIADAQ). It retains the original UI interface and corresponding functionality while adding online features. The online functionality relies on the shared memory mechanism provided by [iceoryx](https://iceoryx.io/latest/), allowing the program to read data from memory in real time and plot it. Given the diversity of experimental setups, this project cannot cover all scenarios, so it only provides a framework for online analysis. Users are responsible for implementing their own data analysis and plotting processes.

## Download and Installation

### Dependencies

- **iceoryx**, version 2.06
- **cmake**, version 3.10 or higher
- **ROOT**, version 6.28 or higher
- **[PLXSDK](https://www.broadcom.com/products/pcie-switches-retimers/software-dev-kits)**, version 8.23

Apart from specific version requirements for `iceoryx` and `PLXSDK`, there are no strict requirements for `cmake` and `ROOT` as long as the project can compile and run. `PLXSDK` is the driver for the XIA hardware acquisition card. The official version is linked above, but you can also download the corresponding package from [PKUXIADAQ](https://github.com/wuhongyi/PKUXIADAQ) or use the version provided by [XIA](https://github.com/xiallc/broadcom_pci_pcie_sdk). All three versions are identical.

### PLXSDK

After downloading, extract the `PlxSdk` to your preferred directory and set the directory as an environment variable:

```bash
export PLX_SDK_DIR="/path/to/PlxSdk"
```

You can save this command to your `~/.bashrc`.

#### Compile PlxApi

```bash
# Navigate to the PLXSDK directory
cd ${PLX_SDK_DIR}
# Enter the PlxApi directory
pushd PlxApi
# Clean up previous builds
make clean
# Compile PlxApi
make
# Return to the previous directory
popd
```

#### Test PlxApi

```bash
# Navigate to the test directory
pushd Samples/ApiTest
# Clean up previous builds
make clean
# Run tests
make
# Return to the previous directory
popd
```

#### Compile the driver

```bash
# Navigate to the driver directory
pushd Driver
# Compile the driver
bash builddriver 9054
# Return to the previous directory
popd
```

If you encounter issues during the compilation process, refer to the documentation of [PKUXIADAQ](http://wuhongyi.cn/PKUXIADAQ/zh/INSTALL.html). For PLXSDK downloaded from the official website, if you are using Ubuntu 20.04 or 22.04 and encounter compilation errors, refer to [Compiling PLXSDK on Ubuntu](https://kinstaky.github.io/xia-daq-gui-online/latest/compile_plxsdk_ubuntu/).

### Download the Project

Clone the repository in version v1.0.1 from GitHub:

```bash
git clone --branch v1.0.1 --single-branch https://github.com/kinstaky/xia-daq-gui-online.git
```

### Build

Navigate to the project directory:

```bash
cd xia-daq-gui-online
```

Use `cmake` to configure the build, specifying the source directory (`.`) and outputting the build files to the `build` folder:

```bash
cmake -S. -Bbuild
```

Compile using 4 threads:

```bash
cmake --build build -- -j4
```

## Getting Started

This project is only a framework. For actual use, it needs to be integrated with XIA for data acquisition, so it does not include functional online programs for real hardware or detectors.  

To test the online framework and software itself, refer to the example and test section below. It includes a simulation system and corresponding online program.  

To apply this framework to real acquisition systems and detectors, consult the [Getting Started](https://kinstaky.github.io/xia-daq-gui-online/latest/getting_started/).

## Examples and Testing

The project includes an example in the `examples` directory. After compilation, two programs can be found in the `build/examples` directory:

- `alpha_source_dssd_simulation`
- `online_example`

In brief:

- `alpha_source_dssd_simulation` simulates measuring a \(3\alpha\) source using a \(32 \times 32\) double-sided silicon strip detector (DSSD) and simulates XIA data acquisition with continuous data output.
- `online_example` is an example of an online program that displays test data in real time.

### Running for Test

1. Open a new terminal and run:

   ```bash
   iox-roudi
   ```

   This program, provided by `iceoryx`, sets up a shared memory platform required by other programs dependent on `iceoryx`. If the program is missing or fails to run, refer to the [iceoryx documentation](https://iceoryx.io/latest/).

2. Open a second terminal and run:

   ```bash
   ./build/examples/alpha_source_dssd_simulation
   ```

   This program simulates data for the online program.

3. Open a third terminal and run:

   ```bash
   ./build/examples/online_example -r 0 -c 0
   ```

   This program is the actual online application. A ROOT window will open, displaying a real-time updating plot, as shown below:

   ![Online Example](https://github.com/kinstaky/xia-daq-gui-online/blob/master/docs/docs/images/online_example.png)

#### Explanation of Parameters

- `-r 0`: Specifies **run 0**.
- `-c 0`: Specifies **crate 0**.

These parameters should be adjusted based on the actual experimental setup. For this example, no changes are needed.

## Documentation

Full documentation is available on https://kinstaky.github.io/xia-daq-gui-online/latest/