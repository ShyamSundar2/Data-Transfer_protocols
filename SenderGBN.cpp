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

#define PORT1 8093
#define MAXLINE 1000

bool Debug_Mode = false;
string Receiver_Name;
int Receiver_Port;
int Seq_Num_Len ;
int Max_Packet_Len ;
int Packet_Gen_Rate ;
int Max_Packets ;
int Window_Size ;
int Buffer_Size ;
int Max_Seq_No ;

int Packet_Seq_No = 0 ;
int sockfd;
int Total_Retranmsissions = 0;

int s_n = 0;

double TimeOut_time ;
double Sum_RTT = 0;

bool Terminate = false;

pthread_mutex_t MLock;

default_random_engine gen;

template<typename... T>
void wt(T... args)
{
	((cout << args <<" "), ...);
	cout<<endl;
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

Timer gt;

struct Packet{
    int seq;
    string Msg;
    Timer t;
    Timer t1;
    double RTT;
    bool acknowledged;
    int Num_Resent;
    double gen_time;
};

deque<Packet> Buffer;
vector<Packet> Sent_Buf;

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
    servaddr.sin_port = htons(PORT1);

    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

}


/*Used To Send the given message To a router*/
void SEND(string hello)
{
    
    struct sockaddr_in	 cliaddr;
    memset(&cliaddr, 0, sizeof(cliaddr));

    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(Receiver_Port);
    cliaddr.sin_addr.s_addr = inet_addr(Receiver_Name.c_str());
    
    int n;
    socklen_t len;

    int x;
    x = sendto(sockfd, hello.c_str(), strlen(hello.c_str()),MSG_CONFIRM, (const struct sockaddr *) &cliaddr,sizeof(cliaddr));
    // cout<<hello<<" MESSAGE SENT\n";
    // printf("%d Hello message sent.\n",x);
}

void Receive(string &str)
{
    // cout<<"CAME IN REceive\n";
    
    char buffer[MAXLINE];
    struct sockaddr_in cliaddr;
    int n;
    socklen_t len;
    
    memset(&cliaddr, 0, sizeof(cliaddr));
    bzero(buffer,MAXLINE);
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,MSG_WAITALL, (struct sockaddr *) &cliaddr,&len);
    buffer[n] = '\0';
    int i = 0;
    while(buffer[i] != '\0')
        str.push_back(buffer[i]),i++;
    // cout<<buffer<<" "<<str<<"MESSAGE RECIEVED\n";
}

void timer_start(std::function<void(void)> func, unsigned int interval)
{
    std::thread([func, interval]() {
        while (true)
        {
            if(Sent_Buf.size() == Max_Packets)
                break;
            std::this_thread::sleep_for(std::chrono::microseconds(interval));
            func();
        }
    }).detach();
}



void Packet_gen()
{
    // cout<<"PACKET_GEN CALLED\n";
    // for(int i = 0 ; i < Packet_Gen_Rate; i++)
    // {
        if(Buffer.size() >= Buffer_Size)
            return ;
        string s = to_string(Packet_Seq_No) + " ";
        int Size = Max_Packet_Len;

        string buf(Size - s.size(),'*');

        s += buf;

        Packet p;
        p.Msg = s;
        p.seq = Packet_Seq_No;
        p.Num_Resent = 0;
        p.RTT = 0;
        p.gen_time = gt.elapsed();

        Buffer.push_back(p);

        if(Packet_Seq_No == Max_Seq_No)
            Packet_Seq_No = 0;
        else
            Packet_Seq_No ++ ;    
        
        // cout<<s<<endl;
    // }
}

void Send_msg()
{
    // cout<<"SENDER STARTED SENDING MESSAGE\n";
    while(1)
    {
        
       if(Sent_Buf.size() == Max_Packets)
            break;
        pthread_mutex_lock(&MLock);

        if(Buffer.size() == 0 || s_n >= min((int)Buffer.size(),Window_Size))
        {
            pthread_mutex_unlock(&MLock);
            continue;
        }
        SEND(Buffer[s_n].Msg);
        Buffer[s_n].t.start();
        if(Buffer[s_n].Num_Resent == 0)
            Buffer[s_n].t1.start();  
        Buffer[s_n].Num_Resent++;
        Total_Retranmsissions++;
        if(Buffer[s_n].Num_Resent >= 10)
        {
            // cout<<"Retransmissons exceeded 10 for a Packet\n";
            Terminate = true;
            pthread_mutex_unlock(&MLock);
            return ;
        }
        s_n++;

         pthread_mutex_unlock(&MLock);
    }
}

void Timeout_check()
{

    // cout<<"TIME OUT STARTED\n";
    
    while(1)
    {
        if(Sent_Buf.size() == Max_Packets || Terminate)
            break;

        if(Sent_Buf.size() >= 10)
            TimeOut_time = (2*Sum_RTT)/Sent_Buf.size();

        pthread_mutex_lock(&MLock);

        if(Buffer.size() == 0)
        {
            pthread_mutex_unlock(&MLock);
            continue;
        }
        if(Buffer[0].t.elapsed() >= TimeOut_time)
        {
            // cout<<"TIMEOUT HAPPEND FOR"<<i<<"\n";
            s_n = 0;
        }

        pthread_mutex_unlock(&MLock);
    }
}

void Receive_Ack()
{
    // cout<<"SENDER STARTED RECIVEING ACK\n";
    while(1)
    {
        // cout<<"CAME IN LOOP\n";
        if(Sent_Buf.size() >= Max_Packets || Terminate)
            break;
        if(Buffer.size() == 0)
            continue;
        string s = "";
        Receive(s);
        istringstream ss(s);

        string s3;
        ss >> s3;
        
        if(s3 == "ACK")
        {
            int Seq;
            ss >> Seq;

            pthread_mutex_lock(&MLock);

            while(Buffer.size()!=0 && Buffer[0].seq != Seq)
            {
                Buffer[0].acknowledged = true;
                Buffer[0].RTT = Buffer[0].t1.elapsed();
                Sum_RTT += Buffer[0].RTT;
                if(Debug_Mode)
                    cout<<"Seq "<<Buffer[0].seq<<": Time Generated: "<<int(Buffer[0].gen_time)<<":"<<(int(Buffer[0].gen_time*1000))%1000<<" RTT: "<<Buffer[0].RTT<<" Number of Attempts: "<<Buffer[0].Num_Resent<<endl<<endl;
                Sent_Buf.push_back(Buffer[0]);
                Buffer.pop_front();
                s_n--;
            }

            pthread_mutex_unlock(&MLock);
        }
    }
}

int main(int argc, char *argv[])
{
    Debug_Mode = false;
    Receiver_Name = "127.0.0.1";
    Receiver_Port = 10013;
    Seq_Num_Len = 10;
    Max_Packet_Len = 50;
    Packet_Gen_Rate = 100;
    Max_Packets = 20;
    Window_Size = 4;
    Buffer_Size = 1000;

    gt.start();
    for(int i = 1 ; i < argc ; i++)
    {
        stringstream ss = stringstream(argv[i]);
        string s;
        ss >> s;

        if(i != argc-1)
            ss = stringstream(argv[i+1]); 

        if(s == "-d")
            Debug_Mode = true;

        if(s == "-s")
            ss >> Receiver_Name;

        if(s == "-p")
            ss >> Receiver_Port;
        
        if(s == "-l")
            ss >> Max_Packet_Len;

        if(s == "-r")
            ss >> Packet_Gen_Rate;

        if(s == "-n")
            ss >> Max_Packets;

        if(s == "-w")
            ss >> Window_Size;
        
        if(s == "-b")
            ss >> Buffer_Size;
        
    }

    wt(Debug_Mode,Receiver_Name,Receiver_Port,Max_Packet_Len,Packet_Gen_Rate,Max_Packets,Window_Size,Buffer_Size);

    Max_Seq_No = (1 << Seq_Num_Len) - 1;

    // cout<<Max_Seq_No << endl;

    create_socket();
    TimeOut_time = 300;
    Packet_gen();

    sleep(1);
    // cout<<"SOCKET CREATED\n";
    
    timer_start(Packet_gen,1000000/Packet_Gen_Rate);
    thread(Send_msg).detach();
    thread(Receive_Ack).detach();
    
    Timeout_check();

    // while()

    cout<<"Average RTT: "<<Sum_RTT/Sent_Buf.size()<<"\n";
    cout<<"Retransmission Ratio: "<<Total_Retranmsissions/(double)Sent_Buf.size()<<endl;
    close(sockfd);

    return 0;

}


