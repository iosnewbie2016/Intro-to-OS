# Project README file

This is **YOUR** Readme file.

## Project Description
We will manually review your file looking for:

- A summary description of your project design.  If you wish to use graphics, please simply use a URL to point to a JPG or PNG file that we can review

- Any additional observations that you have about what you've done. Examples:
	- __What created problems for you?__
	- __What tests would you have added to the test suite?__
	- __If you were going to do this project again, how would you improve it?__
	- __If you didn't think something was clear in the documentation, what would you write instead?__

Part 1:
For part 1 the steps I knew that I had to take were to first define the RPC interface. This took a little while to get correctly until I read on piazza to use opaque pointers. Once that ran, I had trouble figuring out how to allocate memory for the structs I defined in my .x file, but I solved it through trial and error. After it was pretty easy to call the actual RPC defined and assign it to the correct return values. Then I had to free the correct pointer I malloc'ed initially, which also took some trial and error but I understand why I didnt have to free one of the objects I malloc'ed. For the server side, I had magickminify do most of the work and it was really simple after seeing how that function is implemented in magickminify_test.c. That code was really helpful. The timeout issue was solved by reading and using code detailing how clnt_control() worked. 


## Known Bugs/Issues/Limitations

__Please tell us what you know doesn't work in this submission__

## References
http://ics.upjs.sk/~novotnyr/home/programovanie/c/books/cpc/xdr_datatypes.htm
http://web.cs.wpi.edu/~rek/DCS/D04/SunRPC.html
https://docs.oracle.com/cd/E19253-01/816-1435/rpcgenpguide-21/index.html
https://docs.oracle.com/cd/E19683-01/816-1435/rpcgenpguide-24243/index.html
https://www.geeksforgeeks.org/double-pointer-pointer-pointer-c/
http://www.c4learn.com/c-programming/c-gcc-program-rpc/
https://cs.nyu.edu/courses/spring00/V22.0480-002/class07.html
https://piazza.com/class/jc6jlz153n3462?cid=1010