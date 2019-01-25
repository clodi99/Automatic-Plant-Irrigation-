//
// Elon Moist / moisture.cpp 
// by Claudio Alonso, Maxine Mheir, Russell Islam
//
//
//
// December 1st, 2017

//Include statements
//////////////////////////////////////////////////////////////
//
//
//
#include <iostream>
#include <stdio.h>      // standard input / output functions
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <fstream>

using namespace std;

////////// Struct definition ////////////

struct Dataset{
    int numData;
    int data[100];
};

struct Statistics{
    int   minimum;
    int average;
    int   maximum;
    float smplStdDev;
};

///////// Function definition ///////////

void createLogFile();

void logging(int errorCode);

static char *getDtTm (char *buff);d 

int computeStatistics(Dataset& data, Statistics& stats);

void minimum (int moisture, Statistics& stats);

void maximum (int moisture, Statistics& stats);

int readMoistureFile(Dataset& data);

bool MoisturePlant(const Statistics& stats);

void WaterPump();

int firstWriteStats(Statistics& stats, int& ReadingCounter, int& dayCounter);

int AppendWriteFile(Statistics& stats, int& ReadingCounter, int& dayCounter);

//////////////////////////////////////////////////////////////
//
// Code
//
/////////////////////////Define declarations//////////////////

#define DTTMFMT "%Y-%m-%d %H:%M:%S"
#define DTTMSZ 21
#define isNaN(X) (X != X)                           // NaN is the only float that is not equal to itself
#define NaN std::numeric_limits<float>::quiet_NaN() // (comes from <limits>)


int readMoistureValues(){

	int SERIAL = open( "/dev/ttyS1", O_RDWR);

	/* Error Handling */
	if ( SERIAL < 0 ){
		cout << "Error " << errno << " opening " << "/dev/ttyS1" << ": " << strerror (errno) << endl;
        logging(18);
		return -1;
	}

	/* *** Configure Port *** */
	struct termios tty;
	memset (&tty, 0, sizeof tty);
    	logging(20); //Adds to log file, if called

	/* Error Handling */
	if ( tcgetattr ( SERIAL, &tty ) != 0 ){
		cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << endl;
        logging(19); //Adds to log file, if called
		return -1;
	}

	/* Set Baud Rate */
	cfsetospeed (&tty, B9600);
	cfsetispeed (&tty, B9600);

	/* Setting other Port Stuff */
	tty.c_cflag     &=  ~PARENB;        						 // Make 8n1
	tty.c_cflag     &=  ~CSTOPB;
	tty.c_cflag     &=  ~CSIZE;
	tty.c_cflag     |=  CS8;
	tty.c_cflag     &=  ~CRTSCTS;       						 // no flow control
	tty.c_lflag     =   0;          						 // no signaling chars, no echo, no canonical processing
	tty.c_oflag     =   0;                  					 // no remapping, no delays
	tty.c_cc[VMIN]      =   0;                  				 	 // read doesn't block
	tty.c_cc[VTIME]     =   5;                  				 	 // 0.5 seconds read timeout

	tty.c_cflag     |=  CREAD | CLOCAL;     					 // turn on READ & ignore ctrl lines
	tty.c_iflag     &=  ~(IXON | IXOFF | IXANY);				 	 // turn off s/w flow ctrl
	tty.c_lflag     &=  ~(ICANON | ECHO | ECHOE | ISIG);         			 // make raw
	tty.c_oflag     &=  ~OPOST;              					 // make raw


	//allocating buffer memory
	char buf [256];
	memset (&buf, '\0', sizeof buf);
    	logging(21); //Adds to log file, if called


	//Creating moisture value file
	ofstream outfile;
    	logging(22); //Adds to log file, if called

	outfile.open("moisture.txt");

	if(!outfile.is_open()){
		cout<< "Error: file not open" << endl;
        	logging(23); //Adds to log file, if called
		return -1;
	}

	//print new reading to file (100 data points)
	for(int i = 0; i < 100; i++){

		//Flush port
		tcflush( SERIAL, TCIFLUSH );

		//error handling for flush
		if ( tcsetattr ( SERIAL, TCSANOW, &tty ) != 0){
			cout << "Error " << errno << " from tcsetattr" << endl;
			logging(24); //Adds to log file, if called
			return -1;
		}
		
		usleep(100);

		//READ
		int n = read( SERIAL, &buf , sizeof buf );

		//Error handling for read
		if (n < 0){
		    cout << "Error reading: " << strerror(errno) << endl;
		    int val = fcntl(SERIAL, F_GETFL, 0);
		    printf("file status = 0x%x\n", val);
		    return -1;
		}

		outfile << buf;
	}
	

	outfile.close();
	close(SERIAL);

	return 0;

}

//READ DATA FROM GENERATED MOISTURE FILE
int readMoistureFile(Dataset& data){

    int currentLineValue = 0;
    fstream myfile;
    myfile.open("moisture.txt");
    
    if (!myfile.is_open()){
        cout << "FAIL" << endl;
        return -1;
    }
    
    char currentline[3];

    for (int i = 0; i < 100; i++){
    	data.data[i] = 0;
    }

    int value;
    value = 0;
    int pos;
    pos = 0;
    
    
    bool done = false;

    while(!done){
    	++currentLineValue;

        if (!myfile.getline(currentline, 4)){
            if (myfile.eof()){
                done = true;
                break;
            }
            else{
                return -1;
            }
        }
        else{
        	if (currentLineValue % 2 == 1){

            for (int i = 0; i < 3; i++){

	            if(currentline[i] >= '0' && currentline[i] <= '9'){
	            	value = value*10 + (currentline[i]-48);
	            }
               
            }

            data.data[pos] = value;
            data.numData++;
            pos++;
            value = 0;
        }
    }

	}

    myfile.close();

    return 0;
}

//COMPUTING THE MINIMUM
void minimum(int moisture, Statistics& stats) {
    
    if (moisture<stats.minimum){
        stats.minimum = moisture;
    }
}

//COMPUTING THE MAXIMUM
void maximum (int grade, Statistics& stats){

    if (grade>stats.maximum){
        stats.maximum = grade;
    }
}

//COMPUTING THE DATA (INCLUDING SAMPLE DEV. AND AVERAGE)
int computeStatistics(Dataset& data, Statistics& stats){

    stats.minimum = data.data[0];
    stats.maximum = data.data[0];
    float average;
    average = 0;
    float sum;
    sum = 0;
    
    //IF THE NUMBER OF DATA IF NEGATIVE
    if (data.numData < 0){
        logging(5); //Adds to log file, if called
        return -1;
    }
    
    //COMPUTING DATA
    for (int y=0;y<data.numData;y++){
        minimum(data.data[y],stats);
        maximum(data.data[y],stats);
        average = average + data.data[y];
    }
    stats.average = average/data.numData;//change maybe
    
    //COMPUTING SAMPLE DEV
    for (int m=0;m<data.numData;m++){
        sum += pow(data.data[m]-stats.average,2);
    }
    stats.smplStdDev = sqrt((sum/(data.numData-1)));
    
    //IF MIN OR MAX ARE NEGATIVE
    if (stats.minimum<0||stats.maximum<0){
      	logging(6); //Adds to log file, if called
        return -1;
    }
    logging(7); //Adds to log file, if called
    return 0;
}

//WRITE THE STATS IN A TEXT FILE FOR THE FIRST TIME
int firstWriteStats(Statistics& stats, int& ReadingCounter, int& dayCounter){

    int sizeName = 0;
    bool done = false;
    
  	//CREATING A FILE 
    ofstream outfile;
    outfile.open("statistics.txt");
  
  	//ERROR CHECK IF THE FILE CAN'T OPEN
    if (!outfile.is_open()){
      	logging(10); //Adds to log file, if called
        return -1;
    }

  	//CONTENT OF THE FILE
    outfile<<"Day "<< dayCounter <<endl;
    outfile<<endl;
    outfile <<"Reading # " << ReadingCounter << " of the day"<< endl;
    outfile <<"Minimum: " << stats.minimum << endl;
    outfile <<"Average: " << stats.average << endl;
    outfile <<"Maximum: " << stats.maximum << endl;
    outfile <<"Sample standard deviation: " <<stats.smplStdDev << endl;
    
    if (stats.average<=75){
        outfile << "The plant was watered"<<endl;
    }
    outfile<<endl;
    
  	//CLOSING THE FILE
    outfile.close();
    logging(11); //Adds to log file, if called

    return 0;
}
//APPEND DATA TO THE EXISTING TEXT FILE
int AppendWriteFile(Statistics& stats, int& ReadingCounter, int& dayCounter){

    int sizeName = 0;
    bool done = false;

  	//OPENING THE STATS FILE
    ofstream outfile;
    outfile.open("statistics.txt", fstream::app|fstream::out);
  
  	//ERRROR CHECK IF THE FILE DOESN'T OPEN
    if (!outfile.is_open()){
    	logging(12); //Adds to log file, if called
        return -1;
    }  

  	//CONTENT OF THE FILE
    outfile<<"Day "<< dayCounter <<endl;
    outfile<<endl;
    outfile <<"Reading # " << ReadingCounter << " of the day"<< endl;
    outfile <<"Minimum: " << stats.minimum << endl;
    outfile <<"Average: " << stats.average << endl;
    outfile <<"Maximum: " << stats.maximum << endl;
    outfile <<"Sample standard deviation: " <<stats.smplStdDev << endl;
    
    if (stats.average<=75){
        outfile << "The plant was watered"<<endl;
    }
    outfile<<endl;
    
    //CLOSING THE FILE
    outfile.close();
    logging(13); //Adds to log file, if called

    return 0;
}
//DETERMINE IF THE PLANT REQUIRES WATER OR NOT
bool MoisturePlant (Statistics& stats){

    if (stats.average<=75){
      	logging(8); //Adds to log file, if called
      	return true;
    }
    return false;
}


//SETTING THE WATERPUMP
void WaterPump(){

	system("relay-exp -i 1 0");

	system("relay-exp 1 1");

	bool stopMotor = true;

  	sleep(10);

	system("relay-exp 1 0");

	logging(8);	//Adds to log file, if called		
}

//OBTAINS CURRENT DATE AND TIME
static char *getDtTm (char *buff) {

    time_t t = time (0);
    strftime (buff, DTTMSZ, DTTMFMT, localtime (&t));

    return buff;
}

//INITIALIZES LOG FILE
void createLogFile(){

    ofstream file;
    file.open("log.txt");

    file << "LOG FILE" << endl;
    file << endl;

    file.close();
}

//LOGGING FUNCTION
void logging(int errorCode){

    ofstream filestr;
    char buff[DTTMSZ];

    filestr.open("log.txt", ios::out|ios::app);
    filestr << getDtTm (buff) << " ";

    switch (errorCode){
        case 1:
        filestr << "[FATAL] moisture.cpp: readCSV(): failed to open read file" << endl;
        break;
        case 2:
        filestr << "[ERROR] moisture.cpp: readMoistureFile(): too many incorrect inputs" << endl;
        break;
        case 3:
        filestr << "[WARNING] moisture.cpp: readMoistureFile(): received invalid input" << endl;
        break;
        case 4:
        filestr << "[TRACE] moisture.cpp: readMoistureFile(): file was read" << endl;
        break;
        case 5:
        filestr << "[ERROR] moisture.cpp: computeStatistics(): number of data points is negative" << endl;
        break;
        case 6:
        filestr << "[WARNING] moisture.cpp: computeStatistics(): minimum and/or maximum value is negative" << endl;
        break;
        case 7:
        filestr << "[TRACE] moisture.cpp: computeStatistics(): statistics were computed" << endl;
        break;
        case 8:
        filestr << "[TRACE] moisture.cpp: moisturePlant(): water was added to the plant" << endl;
        break;
        case 10:
        filestr << "[FATAL] moisture.cpp: firstWriteStats(): failed to open write file" << endl;
        break;
        case 11:
        filestr << "[TRACE] moisture.cpp: firstWriteStats(): data was outputted; first write" << endl;
        break;
        case 12:
        filestr << "[FATAL] moisture.cpp: AppendWriteFile(): failed to open write file" << endl;
        break;
        case 13:
        filestr << "[TRACE] moisture.cpp: appendWriteFile(): data was outputted; append write" << endl;
        break;
        case 14:
        filestr << "[FATAL] moisture.cpp: readMoistureFile(): failed to read csv file" << endl;
        break;
        case 15:
        filestr << "[FATAL] moisture.cpp: computeStatistics(): failed to compute statistics" << endl;
        break;
        case 16:
        filestr << "[FATAL] moisture.cpp: firstWriteStats(): failed to write to text file" << endl;
        break;
        case 17:
        filestr << "[FATAL] moisture.cpp: AppendWriteFile(): failed to append to text file" << endl;
        break;
        case 18:
      	filestr << "[FATAL] moisture.cpp: readMoistureValues(): failed to read from serial port" << endl;
      	break;
      	case 19:
      	filestr << "[FATAL] moisture.cpp: readMoistureValues(): failed to initialize ports" << endl;
        break;
    	case 20:
      	filestr << "[TRACE] moisture.cpp: readMoistureValues(): initialized and allocated memory for serial ports" << endl;
      	break;
      	case 21:
      	filestr << "[TRACE] moisture.cpp: readMoistureValues(): allocated buffer memory" << endl;
    	break;
      	case 22:
      	filestr << "[TRACE] moisture.cpp: readMoistureValues(): created moisture file successfully" << endl;
      	break;
      	case 23:
      	filestr << "[FATAL] moisture.cpp: readMoistureValues(): failed to open moisture.txt" << endl;
      	break;
      	case 24:
      	filestr << "[ERROR] moisture.cpp: readMoistureValues(): failed to clear flush port" << endl;
      	break;
      	case 25:
      	filestr << "[TRACE] moisture.cpp: readMoistureValues(): reading moisture values was successful" << endl;
      	break;
      	case 26:
      	filestr << "[TRACE] moisture.cpp: WaterPump(): waterPump function returned true" << endl;
      	break;
      	case 27:
      	filestr << "[TRACE] moisture.cpp: timeTillDue(): tillTillDue function returned difference" << endl;
      	break;
        case 0:
        filestr << "[TRACE] moisture.cpp: main(): exited with value 0" << endl;
        filestr << endl;
        break;
    }
    filestr.close();
}

int main(const int argc, const char* const argv[]) {

    createLogFile();

    //VARIABLE DECLARATION//
    int ReadingCounter = 1;
    int dayCounter = 1;
    bool firstWrite = true;
    bool stop = true;
	
    while (stop){

		Dataset data = {0};
		Statistics stats = {0};
      
			//READING VALUES FROM THE MOISTURE SENSOR
			int moistureRead = readMoistureValues();

            //ERROR HANDELING READING FROM SENSOR
			if(moistureRead < 0){
				cout<< "ERROR"<<endl;
				stop = false;
			} else {
				cout<<"WORKED"<<endl;
				//stop = false; /*TEMPORARY STOP REMOVE LATER*/

			}
      
        if (readMoistureFile(data)<0){
            cerr<<"Error : Could not read the file"<<endl;
            stop = false;
    	}

    	if (computeStatistics(data, stats)<0){
            cerr<<"Error : Could not compute statistics"<<endl;
            stop = false;
    	}

    	if (firstWrite){
            if (firstWriteStats(stats,ReadingCounter, dayCounter)<0){
                cerr<<"Error : Could not write a text file"<<endl;
                stop = false;
        }

        firstWrite = false;

    	}
        else if (!firstWrite){
            if (AppendWriteFile(stats,ReadingCounter, dayCounter)<0){
            cerr<<"Error : Could not write a text file"<<endl;
            stop = false;
        }

        firstWrite = false;
    	}
      
        //ADDING WATER IF IT NEEDS TO BE ADDED//
        if(stats.average<=75){
            WaterPump();
        }
		
	 	ReadingCounter++;
	 	if (ReadingCounter==25){
	 		dayCounter++;
	 		ReadingCounter = 0;
	 	}

		logging(0); //Adds to log file, if called
	 	sleep(60); 
	 }

	return 0;
}
