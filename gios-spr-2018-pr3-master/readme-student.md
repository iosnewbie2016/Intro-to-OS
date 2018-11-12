
# Project README file

This is **YOUR** Readme file.

## Project Description
We will manually review your file looking for:

- A summary description of your project design.  If you wish to use grapics, please simply use a URL to point to a JPG or PNG file that we can review

- Any additional observations that you have about what you've done. Examples:
	- __What created problems for you?__
	- __What tests would you have added to the test suite?__
	- __If you were going to do this project again, how would you improve it?__
	- __If you didn't think something was clear in the documentation, what would you write instead?__

Part 1:
This part of the project design involved taking that the concept in our GETFILE protocol is parallel to the libcurl handling of the HTTP protocol. I mirrored a lot of what the provided code within handle_with_file did and added a dynamic URL path that would figure out the length of the file and send the header back to the client with the GETFILE protocol. Then it would send the file in chunks with another call to curl_easy_perform. The main thing I struggled was that in my environment handle_with_file.c would not compile or run correctly for some reason. My environment also had to be reconfigured as I lost all my files as I was doing this project. I tested locally this part with a shell script provided in piazza by Hope Miller. 

## Known Bugs/Issues/Limitations

__Please tell us what you know doesn't work in this submission__

## References

__Please include references to any external materials that you used in your project__

Part 1 Helpful Links:
https://www.hackthissite.org/articles/read/1078
https://curl.haxx.se/libcurl/c/getinmemory.html
https://curl.haxx.se/libcurl/c/curl_easy_setopt.html
https://curl.haxx.se/libcurl/c/curl_easy_perform.html
https://piazza.com/class/jc6jlz153n3462?cid=726