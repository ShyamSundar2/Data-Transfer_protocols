#include <bits/stdc++.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <functional>
#include <filesystem>
#include <random>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include<time.h>
#include <mutex> 
using namespace std;

#define PORT1 8081
#define MAXLINE 1000

bool Debug_Mode = false;
int Receiver_Port;
int Seq_Num_Len ;
double Packet_Error_Rate ;
int Max_Packets ;
int Window_Size ;
int Buffer_Size ;
int Max_Seq_No ;
int Num_acknowledged = 0;

int Packet_Seq_No = 0 ;
int sockfd;

int R_n = 0;
int Seq_it = 0;

int TimeOut_time ;

struct sockaddr_in	 clientaddr;
bool cflag = true;


default_random_engine gen;

template<typename... T>
void wt(T... args)
{
	((cout << args <<" "), ...);
	cout<<endl;
}

struct Packet{
    int seq;
    string Msg;
    bool received ;
    double Receive_Time;
};

Packet Buf[10000];

bool DROP()
{
    return (rand() /(double) RAND_MAX) < Packet_Error_Rate;
}

class Timer
{
private:
	// Type aliases to make accessing nested type easier
	using clock_t = std::chrono::high_resolution_clock;
	using second_t = std::chrono::duration<double, std::ratio<1> >;
	
	std::chrono::time_point<clock_t> m_beg;
    bool started ;
 
public:

    Timer()
    {
        started = false;
    }

	void start()
	{
		m_beg = clock_t::now();
        started = true;
	}
	
	double elapsed() const
	{
        if(started)
		    return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count()*1000;
        else
            return 0;
	}
};
Timer t;

void timer_start(std::function<void(void)> func, unsigned int interval)
{
    std::thread([func, interval]() {
        while (true)
        {
            func();
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }).detach();
}


/* Creates a Socket according to flag */
void create_socket()
{
      struct sockaddr_in servaddr;
      
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
      
    memset(&servaddr, 0, sizeof(servaddr));
      
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(Receiver_Port);

    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

}


/*Used To Send the given message To a router*/
void SEND(string hello)
{   
    int n;
    socklen_t len;

    int x;
    x = sendto(sockfd, hello.c_str(), strlen(hello.c_str()),MSG_CONFIRM, (const struct sockaddr *) &clientaddr,sizeof(clientaddr));
        
    // cout<<"RECEIVER SENT "<<hello<<endl;
}

void Receive(string &str)
{
    char buffer[MAXLINE];
    int n;
    socklen_t len;
    memset(&clientaddr, 0, sizeof(clientaddr));
    bzero(buffer,MAXLINE);
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &clientaddr,&len);
    buffer[n] = '\0';

    for(int i = 0;i<n;i++)
        str.push_back(buffer[i]);

    // cout<<"RECEIVER"<<str<<"MESSAGE RECIEVED\n";
}

int next(int seq)
{
    return (seq+1)%(Max_Seq_No+1);
}

bool check(int seq,int R_n)
{
    int x = R_n;
    for(int i = 0; i < Window_Size; i++ )
    {
        if(x == seq)
            return true;
        x = next(x);
    }
    if(R_n!=0 && Buf[seq+Seq_it*(Max_Seq_No+1)].received)
    {
        string s1 = "ACK " + to_string(seq);
        SEND(s1);
    }
    else if(R_n == 0 && Buf[seq+(Seq_it-1)*(Max_Seq_No+1)].received)
    {
        string s1 = "ACK " + to_string(seq);
        SEND(s1);
    }
 return false;
}


void Receive_Msg()
{
    cout<<"RECEIVER STARTED RECEIVING\n";
    while(1)
    {
        if(Num_acknowledged >= Max_Packets)
            break;
        Packet p;
        string s = "";
        Receive(s);
        istringstream ss(s);

        ss >> p.seq;
        ss >> p.Msg;
        p.received = true;

        if(!check(p.seq,R_n) || DROP())
            continue;
        
        p.Receive_Time = t.elapsed();
        if(p.seq == R_n)
        {
            // cout<<"RECEIVER YES R_n ="<<R_n<<"\n";
            Buf[R_n + (Max_Seq_No+1)*Seq_it] = p;
            while(1)
            {
                if(R_n == Max_Seq_No)
                {
                    Seq_it++;
                    R_n = 0;
                }
                else
                    R_n++;
        
                if(!Buf[R_n + (Max_Seq_No+1)*Seq_it].received)
                    break;
            }
            string s1 = "ACK " + to_string(p.seq);
            SEND(s1);
            Num_acknowledged++;
            // cout<<"RECIVER SENT "<<s1<<endl;
        }
        else
        {
            // cout<<"RECEIVER NO R_n = "<<R_n<<"\n";
            Buf[p.seq + (Max_Seq_No+1)*Seq_it] = p;
            string s1 = "ACK " + to_string(p.seq);
            SEND(s1);
            Num_acknowledged++;
        }
    }
}

int main(int argc, char *argv[])
{
    Debug_Mode = false;
    Receiver_Port = 10000;
    Seq_Num_Len = 4;
    Packet_Error_Rate = 0.2;
    Max_Packets = 20;
    Window_Size = 4;
    Buffer_Size = 100;
    t.start();
    for(int i = 1 ; i < argc ; i++)
    {
        stringstream ss = stringstream(argv[i]);
        string s;
        ss >> s;

        if(i != argc-1)
            ss = stringstream(argv[i+1]); 

        if(s == "-d")
            Debug_Mode = true;

        if(s == "-p")
            ss >> Receiver_Port;

        if(s == "-n")
            ss >> Seq_Num_Len;
        
        if(s == "-e")
            ss >> Packet_Error_Rate;

        if(s == "-N")
            ss >> Max_Packets;

        if(s == "-W")
            ss >> Window_Size;
        
        if(s == "-B")
            ss >> Buffer_Size;
        
    }

    wt(Debug_Mode,Receiver_Port,Seq_Num_Len,Packet_Error_Rate,Max_Packets,Window_Size,Buffer_Size);

    Max_Seq_No = (1 << Seq_Num_Len) - 1;

    cout<<Max_Seq_No << endl;

    for(int i = 0; i < Max_Packets ;i++ )
    {
        Buf[i].Msg = "*";
        Buf[i].seq = i%(Max_Seq_No+1);
        Buf[i].received = false;
    }

    create_socket();
    // sleep(1);
    cout<<"SOCKET CREATED\n";

    Receive_Msg();
    int index = 0;
    for(int i = 0; index < Max_Packets ; i++)
    {
        if(Debug_Mode && Buf[i].received)
        {
            cout<<"Seq "<<Buf[i].seq<<": Time Received: "<<int(Buf[i].Receive_Time)<<":"<<(int(Buf[i].Receive_Time*1000))%1000<<endl,index++;
        }
    } 
    close(sockfd);
    return 0;

}


