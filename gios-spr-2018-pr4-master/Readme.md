# RPC: Remote Procedure Call

## Foreword
In this project, you will work with Remote Procedure Calls, specifically with
Sun RPC. The RPC server will accept a jpeg image as input, downsample it to a
lower resolution, and return the resulting image.

## Setup

You can clone the code in the project 4 repository with the command

```
git clone https://github.gatech.edu/gios-spr-2018/pr4.git
```

Note: if you choose to **fork** the repository, please remove the class access
to it.  You can do this by going to the "Settings" (on github.gatech.edu) for
your repository.  On the left there is a tab for "Collaborators and Teams".  When
you choose this it will show the class team membership.  Simply remove it (there is
an "x" on the right hand side).   Your repository is now private and only accessible
to you and class instructors.

**If you do not remove team access, your code is visible to everyone in class.  This
is the same as sharing your code.**

## Submission Instructions
All code should be submitted through the submit.py script given at the top
level of the corresponding folder in the repository.  For instructions on how to submit individual
components of the assignment see the instructions below.  

> For this assignment, you may submit your code up to 10 times in a 24 hour period. After the deadline, we download your last submission prior to the deadline, review your submission and assign a grade. There is no limit to the number of times you may
submit your **readme-student.md** file.

After submitting, you may double-check the results of your submission by
visiting the [Udacity/GT autograding website](https://bonnie.udacity.com) and
going to the student portal.


## README
Throughout the project, we encourage you to keep notes on what you have done,
how you have approached each part of the project and what resources you used in
preparing your work.  We have provided you with a prototype file,
**readme-student.md** that you should use throughout the project.

You may submit your **readme-student.md** file with the command
```
python submit.py readme
```

At the prompt, please provide your GT username and password.

If this is your first time submitting, the program will then ask you if you want
to save the jwt.  If you reply yes, it will save a token on your filesystem so that you don't have to provide your
username and password each time that you submit.

The Udacity site will store your **readme-student.md** file in a database.
This will be used during grading.  The submit script will acknowledge receipt
of your README file. For this project, like in Project 3, you only need to submit one
README file for both parts of the project.

Note: you may submit a PDF version of this file, **readme-student.pdf** in addition to the markdown version.  The
submission script will automatically detect and submit this file, if it is present in the project root directory.  If you submit
both files, we will give the PDF version preference.

> For this assignment, you may submit your code up to 10 times in a 24 hour period. After the deadline, we download your last submission prior to the deadline, review your submission and assign a grade. There is no limit to the number of times you may
submit your **readme-student.md** file.

## Directions
Begin programming by modifying only those specified below.

## Building a Single-Threaded RPC Server
Your first task will be to define the RPC interface through the XDR file
**minifyjpeg.x**.  The syntax and semantics of this type files are explained in
the course videos as well as the documentation for `rpcgen`.

The `rpcgen` tool will generate much of the C code that you will need.  You should
use the -MN option for the "multi-threaded" support and the “newer” rpc style that allows for multiple arguments.

In the generated file **minifyjpeg.h**, you will find the definitions of
functions that you will need to implement.  On the server side, you need to
take the input image, reduce its resolution by a factor of 2, and return the
result.  A library that does this with ImageMagick is provided in the files
**magickminify.[ch]**. For the sake of this project only JPEG (JPG) images will be
used.

On the client side, the provided file minifyjpeg_main.c acts as a workload
generator for the RPC server.  It calls two functions which you should
implement in the file minify_via_rpc.c.

- `get_minify_client` - should connect to the rpc server and return the CLIENT*
	pointer.
- `minify_via_rpc` - should call the remote procedure on the client specified
	as a parameter.

In order to support **images of arbitrary size** and not worry about
implementing packet ordering, the communication should use TCP as the transport
protocol. Keep in mind that you should not be limiting the size of the image
file. This means you have to pay attention to how you define the data
structures in your ".x" file to support files of any size.

Here is a summary of the relevant files and their roles.

- **Makefile** - (do not modify) file used to compile the code.  Run ‘make
	minifyjpeg_main’  and ‘make minifyjpeg_svc’ to compile the client and server
	programs.
- **magickminify.[ch]** - (do not modify) a library with a simple interface
	that downsamples jpeg files to half their original resolution.
- **minify_via_rpc.c** - (modify) implement the client-side functions here so
	that the client sends the input file to server for downsampling.
- **minifyjpeg.h** - (to be generated by rpcgen from minifyjpeg.x) you will
	need to implement the server-side functions listed here in minifyjpeg.c
- **minifyjpeg.c** - (modify) implement the needed server-side function here.
- **minifyjpeg.x** - (modify) define the RPC/XDR interface here.
- **minifyjpeg_clnt.c** - (to be generated by rpcgen from minifyjpeg.x)
	contains the client side code that executes the RPC call.
- **minifyjpeg_main.c** - (do not modify) a workload generator for the RPC
	server.
- **minifyjpeg_svc.c** - (to be generated by rpcgen from minifyjpeg.x) this
	contains the entry point for the server code.  You will need to modify 
        this file to make your code **multithreaded.**
- **minifyjpeg_xdr.c** - (to be generated by rpcgen from minifyjpeg.x) contains
	code related to data structures defined in minifyjpeg.x
- **workload.txt** - (modify for your own testing purposes) an example workload
	file that can be passed into the client main program minifyjpeg_main.c

![RPC Diagram](docs/pr4.png)


## Building a Multi-Threaded RPC Server
For this part, you will modify the server-side file **minifyjpeg_svc.c** 
to make the server multithreaded.  Note that it is the `svc_getargs`
function that copies global data from the rpc library into memory that you
control.  This must be done before the function registered with `svc_register`
returns.  The function `svc_sendreply` can be called later.


**NOTE:** This is one of the generated files, but for this part you can ignore the warning at the top of the file about not editing it. You will have to change the **minifyjpeg_svc.c** main method to accept a command-line parameter (-t) to indicate the number of threads to be created. This will be similar to what you have done in P1 and P3 when it comes to creating a pool of threads. You can copy the argument parsing logic from one of those projects, or you can write your own. In either case your **minifyjpeg_svc.c** main function will receive a command line parameter (-t) to indicate the number of threads to be used. The autograder will test to ensure you are creating the requested number of threads. The syntax used to run the service will be:  
```
minify_svc -t X
```
Where **X** is the number of threads to be created by the main method. No other command-line parameters will be passed to it, so you can write your code assuming that you only need to handle that parameter. 

To submit the multi-threaded portion, please place your code in the **rpc_mt** directory and run the **submit.py** script in that
directory. The command to submit the multi-threaded portion is:
```
python submit.py rpc_mt
```

**Please keep in mind that only the *submit.py* located in the *rpc_mt* directory will submit and grade the MT portion of your code.**

## References

### Relevant Lecture Material

- [P4L1 Remote Procedure
	Calls](https://www.udacity.com/course/viewer#!/c-ud923/l-3450238825)

### RPC Material
- [Sun RPC Example](http://web.cs.wpi.edu/~rek/DCS/D04/SunRPC.html)
- [Sun RPC
	Documentation](https://docs.oracle.com/cd/E19683-01/816-1435/index.html)
	(maintained by Oracle; similar semantics, updated API.)
- [From C to RPC Tutorial](http://www.cs.cf.ac.uk/Dave/C/node34.html)

### Project Design Recommendations
When testing and debugging, start with running small/light client workloads If
your server is slow in responding, you may start timing out at the TCP socket
and then the RPC runtime layer -- thus, look for options to change the timeout
values used with RPCs.

For the timeout test, you need to make sure that you modify the default timeout value
that is generated by RPC since it is larger than the timeout value for the grader. Next you
need to detect the timeout condition and return the appropriate code (*hint* look at the return
codes from RPC and return the one that indicates a timeout).

**NOTE:** RPC will setup a default timeout value in the minifyjpeg_clnt.c file. This file is generated each time you run "rpcgen" and the default value put back in place. The minifyjpeg_clnt.c file has a comment with a hint on how to set the timeout programmatically, so that it doesn't get overwritten.

## Rubric

Your project will be graded at least on the following items:

- Interface specification (.x)
- Service implementation
- RPC initiation and binding
- Proper clean up of memory and RPC resources
- Proper communication with the server
- Proper handling of a server that takes too long to complete the request. The client should time out after between 5 and 10 seconds
- Insightful observations in the Readme file and suggestions for improving the class for future semesters

### RPC Single-Threaded (40 points)
### RPC Multi-Threaded (50 points)
### Readme file (10 points)

## Questions
For all questions, please use the class Piazza forum or the class Slack channel so that TA's and other students
can assist you.
