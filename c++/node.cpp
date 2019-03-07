#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include<pthread.h>

using namespace std;
typedef struct node {
    char name[20];
    char ip[50];
    int port;
    int cost;
}node;
typedef struct routing_table {
    char name[20];
    int port;
    node node1;
    node node2;
}routing_table;

std::vector<std::string> split(const std::string &str, const std::string &delim)
{
    std::vector<std::string> spiltCollection;
    if(str.size()==0)
        return spiltCollection;
    int start = 0;
    int idx = str.find(delim, start);
    while( idx != std::string::npos )
    {
        spiltCollection.push_back(str.substr(start, idx-start));
        start = idx+delim.size();
        idx = str.find(delim, start);
    }
    spiltCollection.push_back(str.substr(start));
    return spiltCollection;
}

void print_table(routing_table table){
    printf(">>> %s Routing table<<<\n",table.name);
    printf("-------------------------------------------------------\n");
    printf("|   destination   |    link cost    |    next hop     |\n");
    printf("|    %-13s|    %-13d|    %-13s|\n",table.node1.name,table.node1.cost,table.node1.name);
    printf("|    %-13s|    %-13d|    %-13s|\n",table.node2.name,table.node2.cost,table.node2.name);
    printf("-------------------------------------------------------\n");
}

routing_table read(char* file)
{
    routing_table ret;
    ifstream in(file);
    if (! in.is_open())
    {
        cout<<"Can not open config file "<<file<<endl;
        return ret;
    }
    int line = 0;
    while (!in.eof() )
    {
        char buffer[1024];
        in.getline (buffer,100);
        line++;
        vector<string> array;
        string s = string(buffer);
        array = split(s.c_str(),":");
        if(line == 1)
        {
            sprintf(ret.name,"%s",array[0].c_str());
            ret.port = atoi(array[1].c_str());
        }
        if(line == 2)
        {
            sprintf(ret.node1.name,"%s",array[0].c_str());
            sprintf(ret.node1.ip,"%s",array[1].c_str());
            ret.node1.port = atoi(array[2].c_str());
            ret.node1.cost = atoi(array[3].c_str());
        }
        if(line == 3)
        {
            sprintf(ret.node2.name,"%s",array[0].c_str());
            sprintf(ret.node2.ip,"%s",array[1].c_str());
            ret.node2.port = atoi(array[2].c_str());
            ret.node2.cost = atoi(array[3].c_str());
        }
    }
    return ret;
}

void sendto(char* ip,int port,routing_table table)
{
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr_serv;
    memset(&addr_serv, 0, sizeof(addr_serv));
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_addr.s_addr = inet_addr(ip);
    addr_serv.sin_port = htons(port);
    int len = sizeof(addr_serv);
    sendto(sock_fd, &table, sizeof(routing_table), 0, (struct sockaddr *)&addr_serv, len);
}

void* server_func(void* arg)
{
    routing_table table;
    memcpy(&table,arg,sizeof(routing_table));
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr_serv;
    int len;
    memset(&addr_serv, 0, sizeof(struct sockaddr_in));
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(table.port);
    addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);
    len = sizeof(addr_serv);
    if(bind(sock_fd, (struct sockaddr *)&addr_serv, sizeof(addr_serv)) < 0)
    {
        perror("bind error:");
        exit(1);
    }
    struct sockaddr_in addr_client;

    while(1)
    {
        routing_table recv;
        char buffer[1024];
        int recv_num = recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);
        memcpy(&recv,buffer,sizeof(routing_table));
        print_table(recv);
    }
}

int main(int argc,char* argv[])
{
    if(argc != 2)
    {
        cout<<"Useage:"<<"./"<<argv[0]<<" <config file>"<<endl;
        return -1;
    }
    routing_table table = read(argv[1]);
    //server thread
    pthread_t tid;
    int ret = pthread_create(&tid, NULL,server_func,&table);
    while(1)
    {
        cout<<"Input command(FirstLoad,FirstSend,Bye, or MyRoutingTable):"<<endl;
        string cmd;
        getline(cin,cmd);
        if(cmd == "Bye")
            break;
        if(cmd == "FirstSend")
        {
            sendto(table.node1.ip,table.node1.port,table);
            sendto(table.node2.ip,table.node2.port,table);
            cout<<"Send routing table finished!"<<endl;

        }else if(cmd == "MyRoutingTable")
        {
            print_table(table);
        }else if(cmd == "FirstLoad")
        {
            table = read(argv[1]);
            cout<<"Reload config finished!"<<endl;
        }else{
            cout<<"Invalid command!"<<endl;
        }
    }
}
