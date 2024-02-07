# 4061 Project 1 - Basic Autograder
  
## Team - 26
- Jacob Schwantes [schw2550]
- Nadir Mustafa [musta099]
- Adam El-Kishawy [elkis005]    

## Prerequisites

Before proceeding, ensure the following tools are installed and accessible from your command line or terminal:

- **GNU Make**
- **GCC (GNU Compiler Collection)**

## Usage
### Compile
```
make all N=number of submissions to generate
```
### Run
Pass arguments `B` for batch size, `Pi` for test inputs as integers.
```
./autograder <B> <P1> <P2> ...
```

## Assumptions
- Number of submissions should be a multiple of the batch size `B`.
- A process receives a score of (blocked) if it is sleeping and has not exited after 10 seconds.
- A process receives a score of (slow) if it exits after 10 seconds, per the `template.c` spec.
- A process receives a score of (infinite) if it is running and has not exited after 10 seconds.
- Child processes are batched by running different submissions in parallel with the same input parameter.