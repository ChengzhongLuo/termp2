1) Chengzhong Luo
2) 5963319108
3) ubuntu 20.04.5
4) I code a main server and a backend server. The backend servers read from files and store department names and student ids. The main server communicates with the backend servers and gets the department lists. When main server has a query about one department, it will communicate with the corresponding backend server and get the student ids.
5) servermain.cpp is the main server. Firstly, it sends out requests to all three backend servers and wait for them to send back the department lists. I added a time out, so even if not all three backend server is online or not all three backend servers send back the list, the main server will continue after time it up, so that it will not be in an infinite loop waiting. Secondly, it takes in users input, and check if the department exists, then go to the correct backend server to request a student id list. Lastly, it will go over the second step again and again. 
   server.cpp is the backend server. I set different parameter for serverA,B,C in the header file and the makefile, so one server.cpp can generate three different backend servers. For the backend server, it first read a file and store department names and student ids. Then it communicates will the main server like I described above.
6) char
7) I used unsigned long long to store the student id so it can takes up to 19 digits, and I used a large enough buffer. In my own tests, it can send and receive 100 19-digit ids.
8) Reuse the code from Beej's guide