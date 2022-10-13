# Deployment environment
System: Linux Ubuntu 18.04
Language: C++ (gnuplot for GUI, some python codes in ns-3)
Compiler: Waf (python based build system)
Setup: NS-3 emulator based on version 3.35 (requires pre-configuration. 
Please refer to https://www.nsnam.org/docs/release/3.35/tutorial/html/getting-started.html)
The address of the NR module(v1.2): https://gitlab.com/cttc-lena/nr

# System structure
(Make the path ../ns-3.35 the topmost directory)
The placement of nr,scream、mytrace and throughRW folder: ./contrib or ./src
The placement of "screamex.cc" and "scream_ueNum.sh": ./scratch
The placement of VideoSource and traces folder: ./

# Configuration
For configure,build and run, please refer to https://www.nsnam.org/docs/release/3.35/tutorial/html/getting-started.html#configure-vs-build
Multi-user batch operation: "./scratch/scream_ueNum.sh"
Obtaining the user's real throughput that meets the latency requirements(after running the "screamex.cc"): ./throughputRW/examples/throughputRW-example.cc
Rate and Packet Latency Data: ./traces