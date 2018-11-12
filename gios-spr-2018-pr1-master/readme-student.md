# Project README file

This is **YOUR** Readme file.

**Note** - if you prefer, you may submit a PDF version of this readme file.  Simply create a file called **readme-student.pdf** in the same directory.  When you submit, one or the other (or both) of these files will be submitted.  This is entirely optional.

## Project Description
We will manually review your file looking for:

- A summary description of your project design.  If you wish to use graphics, please simply use a URL to point to a JPG or PNG file that we can review

- Any additional observations that you have about what you've done. Examples:
	- __What created problems for you?__
	- __What tests would you have added to the test suite?__
	- __If you were going to do this project again, how would you improve it?__
	- __If you didn't think something was clear in the documentation, what would you write instead?__

Setup:
The first thing I tried to do starting with this project was to set up my environment which caused a lot of headaches working on windows as sometimes vagrant would not work and take up all my hard drive space. This was very frustrating and took up too much of my time. The next thing I tried to re-learn some of the basics from my intro to C programming class as it has been a while since I have programmed in C. I would make a quick clear note about using pointers and passing by value and references along with how to use a Makefile and general stuff like that. 

Warm-up:
Echo:
I started off by reading the beej documents on the socket API to try to implement an echoclient and echoserver. This was very helpful, I ended up using most of the concepts, structures, and system calls included in that reference. I did use the basic socket, connect, and accept skeleton code from this reference when making my echoclient and echoserver. The information about using getaddrinfo was very helpful in just simplifying a way to get addrinfo. What I struggled most is the port was giving me errors for some reason even after applying sprintf to it. I had to define my own value and create a char array that would be accepted by getaddrinfo. My code is heavily commented for these two applications, if you need more information. I tested the application manually by running the server and tested varying lengths of messages and it worked. Specifically for the server, the sockopt function is also part of the code I used from beej.us network programming guide. 

Transfer:
This was largely a continuation on the echo client/server code just with the unique introduction into handling I/O of files and sending bytes to a client in chunks of data and reading in those chunks of data. I again used a get_addr_info helper function (code is just before the main), this is from the beej guide. This took a bit more debugging and thought than the echo client/server. I found myself using a lot of printf() to debug. Some challenges I encountered was how to get the file size which I read a good method on the third reference from stackoverflow, which introduced me to a helpful sys/stat header. After that it took developing the transferclient against a known transferserver binary which I got from piazza which significantly eased this project. I did get some address faults because I would overfill my buffer and definitely struggled with remembering how C would allocate data. After trial and errors, I think I got the code working well without limitations. I used the default files in the folder in my github account. 

Part 1:

GFCLient:

I decided to tackle the client side of this problem first. This was an interesting problem because it included a layer of abstraction that was hard to wrap my head around. I had to read some C examples of libraries and how they abstracted functions defined in the header to allow a user to use the library. Then added on there was a callback function which I have not seen in a while. After wrapping my head around these concepts I made significant progress and was able to reuse a lot of the warm-up material in gfc_perform(). What made this code unique was the ability to create a header for the request and parse the header of the response. I struggled parsing through the request as I was receiving errors about statuses which took a lot of debugging time to get through, and the file written out would for some reason be a byte off. My debugging was very limited to print functions and submitting to Bonnie. I found that the largest struggle was appling a string functions on a buffer. I wrote a workaround that essentially created a a subbuffer and more error checking to remedy this. Then, I found that someone on piazza also had a similar problem and had a more elegant solution after the fact. I would organize piazza a lot better for future projects, such as a thread for gfclient and gfserver it would save a lot of time searching if someone else had a similar problem. I think a lot more resources to C programming would be helpful especially in the Udacity portion, I feel like I have been struggling not on theory but on the actual programming.

GFServer:

The first problem I encountered was to install dos2unix just to have the open file given method work. This problem was less challenging than the client because the requirements of the library were easier since the callback function handler helped. I still had some headaches parsing malformed headers and the fact that the space between path and the terminating sequence was not properly identified in the readme. Once I implemented context structure it was simple to pass variables along. I had trouble with the allocation of the memory and freeing of the memory as I struggled debugging that information. I was ultimately unable to find why my code will not work for large files over 50 MB. I would like some feedback on this aspect. 


Part 2:




## Known Bugs/Issues/Limitations

Echo: NA
Transfer: NA
GFClient: I am not sure how to test if my implementation of gfc_set_headerfunc was correct. 
GFServer: Will not work for large files over 50 MB. I am not sure why.


## References

http://beej.us/guide/bgnet/html/multi/syscalls.html
https://piazza.com/class/jc6jlz153n3462
https://stackoverflow.com/questions/8236/how-do-you-determine-the-size-of-a-file-in-c
https://www.bottomupcs.com/abstration.xhtml
http://www.cprogramming.com/tutorial/function-pointers.html
https://www.tutorialspoint.com/c_standard_library/c_function_sscanf.htm
http://opensourceforu.com/2012/02/function-pointers-and-callbacks-in-c-an-odyssey/
https://learnxinyminutes.com/docs/c/
http://man7.org/tlpi/index.html