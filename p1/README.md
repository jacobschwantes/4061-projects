# 4061 Project 1 - Basic Autograder
  
## Team - 26
- Jacob Schwantes [schw2550]
- Nadir Mustafa [musta099]
- Adam El-Kishawy [elkis005]    

## Contributions
- Jacob: Implemented autograder.c, research, debugging (pair programming)
- Nadir: Helped implement autograder.c, research, debugging (pair programming)
- Adam: Readme, 3.f response

Before proceeding, ensure the following tools are installed and accessible from your command line or terminal:

- **GNU Make**
- **GCC (GNU Compiler Collection)**

## Usage
### Compile
```
make all N=number of submissions to generate
```
### Run
Pass arguments `Pi` for test inputs as integers.
```
./autograder <P1> <P2> ...
```

## Assumptions
- A process receives a score of (blocked) if it is sleeping and has not exited after 2 seconds.
- A process receives a score of (slow) if it exits after 2 seconds, per the `template.c` spec.
- A process receives a score of (infinite) if it is running and has not exited after 2 seconds.
- Child processes are batched by running different submissions in parallel with the same input parameter.

## Q1 Answer
After modifying and simplifying template.c to only return 1, it became apparent how beneficial parallelism can be.
The 10 processes that we created executed almost instantly. However, is seems that parallelism is only beneficial 
when it comes to processes that are doing real computation. With the original template.c file, we are artificially
controlling how each process acts. For example, in the case of a process being stuck/in an infinite loop, we have 
to manually kill the programs. Those cases are not where computation is actually happening. Rather, they are
processes that do no computation and hog the CPU's resources from other processes. This is why the parallelism
benefit is not seen with the original template.c file with the various cases. However, in the simplified ctemplate.c file,
we are not controlling how the processes should act, but instead, we allow them to run the code and behave normally. Which
is why we are able to see the benefit of parallelism and thus a faster execution time.