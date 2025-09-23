/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Matthew Smith
	UIN: 932008265
	Date: 2025/09/19
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg); // atoi = 'alpha to int'
				break;
			case 't':
				t = atof (optarg); /// atof = double to string conversion
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}

	// run server as child process and connect it to FIFO
	pid_t pid = fork();
	if (pid == -1) {
        cerr << "fork failed\n";
        return 1;
    }
	if (pid == 0) {
		// child process, run server
		char* args[] = {(char*) "./server", (char*) nullptr};
		execvp(args[0], args);
		return 1; // if server creation fails
	}

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	
	// example data point request
    // char buf[MAX_MESSAGE]; // 256
    // datamsg x(1, 0.0, 1);
	
	// memcpy(buf, &x, sizeof(datamsg));
	// chan.cwrite(buf, sizeof(datamsg)); // question
	
	// chan.cread(&reply, sizeof(double)); //answer
	// cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;

	// 4.2: Requesting Data Points
	char buf[MAX_MESSAGE]; // 256
    datamsg y(p, t, e);
	
	memcpy(buf, &y, sizeof(datamsg));
	chan.cwrite(buf, sizeof(datamsg)); // question
	double reply;
	chan.cread(&reply, sizeof(double)); //answer
	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;

	// get first 1000 data points for a given person and put them in a new file 
	// TESTME
	int newFd;
	char* newFile[] = {(char*) "received/x1.csv", (char*) nullptr}; // WORKING ON THIS
	newFd = open(*newFile, O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU); // create file in recieved directory
	if (newFd == -1) {
		cerr << "file creation failed\n";
		return 1;
	}
	char recBuf[MAX_MESSAGE]; // buffer to receive data points
	double ecgReply1, ecgReply2; // recieve server replies
	t = 0.0;
	for (int i = 0; i < 1000; i++) {
		// ecg 1
		y.seconds = t;
		y.ecgno = 1;
		memcpy(buf, &y, sizeof(datamsg)); // copy datamsg to buffer
		chan.cwrite(buf, sizeof(datamsg)); // question
		chan.cread(&ecgReply1, sizeof(double)); //answer

		// ecg 2
		y.ecgno = 2;
		memcpy(buf, &y, sizeof(datamsg)); // copy datamsg to buffer
		chan.cwrite(buf, sizeof(datamsg)); // question
		chan.cread(&ecgReply2, sizeof(double)); //answer

		// cout << t << ", " << ecgReply1 << ", " << ecgReply2 << endl; // DEBUG LINE
		// want recBuf to contain time, reply 1, reply 2, then write recBuf to file
		if (i == 999) { // don't add newline on last written line
			sprintf(recBuf, "%.4g%s%.4g%s%.4g", t, ",", ecgReply1, ",", ecgReply2);
		}
		else {
			sprintf(recBuf, "%.4g%s%.4g%s%.4g%s", t, ",", ecgReply1, ",", ecgReply2, "\n");
		}
		// printf("%s", recBuf);
		write(newFd, recBuf, strlen(recBuf));
		t = t + 0.004;
	}

	// sending message
	filemsg fm(0, 0); // message to request file size
	string fname = "nonsense"; 
	// cout << fname << endl;
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];
	memcpy(buf2, &fm, sizeof(filemsg)); // copy fm to buf2
	strcpy(buf2 + sizeof(filemsg), fname.c_str()); // 
	chan.cwrite(buf2, len);  // I want the file length; // send fm to server
	// do something with that
	// create file & copy requested patient file to it

	delete[] buf2;
	
	// closing the channel    
	// cout << "closing server" << endl; // test line
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	// cout << "server closed" << endl; // test line
}
