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
	int p = 0; // default value to avoid unnecessary computation time
	double t = 0.0;
	int e = 0;
	int m = MAX_MESSAGE; 
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:")) != -1) {
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
			case 'm':
				m = atoi (optarg);
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
		char msgSizeChar[32]; // 32 char buffer to convert m from int to char*
		sprintf(msgSizeChar, "%i", m);
		char* args[] = {(char*) "./server", (char*) "-m", (char*) msgSizeChar, (char*) nullptr};
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
	// only do it if data points were requested
	if (p != 0 && e != 0) {
		char* buf = new char[m];
		datamsg x(p, t, e);
		
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		delete[] buf;
	}
	if (p != 0 && e == 0) {
		// get first 1000 data points for a given person and put them in a new file 
		// TESTME
		char* buf = new char[m]; 
		datamsg y(p, t, e);
		int newFd;
		char* newFile[] = {(char*) "received/x1.csv", (char*) nullptr};
		newFd = open(*newFile, O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU); // create file in recieved directory
		if (newFd == -1) {
			cerr << "file creation failed\n";
			return 1;
		}
		char* recBuf = new char[m]; // buffer to receive data points
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
		close(newFd);
		delete[] buf;
		delete[] recBuf;
	}
	
	 
	// only do file request if a file was actually requested
	cout << "filename: " << filename << endl;
	if (filename != "") {
		// sending message
		filemsg fm(0, 0); // message to request file size
		string fname = "default";
		fname = filename;
		// cout << "fname: " << fname << endl; // DEBUG LINE
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg)); // copy fm to buf2
		strcpy(buf2 + sizeof(filemsg), fname.c_str()); // 
		chan.cwrite(buf2, len);  // I want the file length; // send fm to server
		__int64_t fileSize; // get file size from server
		chan.cread(&fileSize, sizeof(__int64_t));
		// create file & copy requested patient file to it
		string copyFileName = "received/";
		copyFileName = copyFileName + filename;
		int copyFD;
		char* copyFile[] = {(char*) copyFileName.c_str(), (char*) nullptr};
		copyFD = open(*copyFile, O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU); // create file in recieved directory
		if (copyFD == -1) {
			cerr << "file copy failed to open!";
			return 1;
		}
		// copy requested file to the new one
		__int64_t remainingBytes = fileSize;
		fm.length = m; // message length
		char* fileBuf = new char[m]; // buffer to receive chunks of file data
		while (remainingBytes > 255) {
			// cout << "rBytes: " << remainingBytes; // DEBUG LINE
			memcpy(buf2, &fm, sizeof(filemsg)); // copy fm to buf2
			strcpy(buf2 + sizeof(filemsg), fname.c_str()); // buf2 contains fm and the name of the desired file
			chan.cwrite(buf2, len); // send fm to server
			chan.cread(fileBuf, m); // receive reply and put it in a buffer
			write(copyFD, fileBuf, m); // write buffer to the file

			remainingBytes = remainingBytes - m; // iterate down by m
			fm.offset = fm.offset + m;
			// cout << "     e      " << remainingBytes << endl; // DEBUG LINE
		}

		// last chunk of data might not be full size
		if (remainingBytes > 0) {
			// cout << "out rBytes: " << remainingBytes << endl; // DEBUG LINE
			fm.length = fileSize % m;
			// cout << "out offset: " << fm.offset << endl; // DEBUG LINE
			memcpy(buf2, &fm, sizeof(filemsg)); // copy fm to buf2
			strcpy(buf2 + sizeof(filemsg), fname.c_str()); // buf2 contains fm and the name of the desired file
			chan.cwrite(buf2, len); // send fm to server
			chan.cread(fileBuf, remainingBytes); // receive reply and put it in a buffer
			write(copyFD, fileBuf, remainingBytes); // write buffer to the file
		}
		
		delete[] buf2;
		delete[] fileBuf;
		close(copyFD);
	}

	// 4.4: Requesting a new channel

	// closing the channel & server
    MESSAGE_TYPE msg = QUIT_MSG;
    chan.cwrite(&msg, sizeof(MESSAGE_TYPE));
}
