#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <map>
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


//global table
std::map<string,routing_table> global_table_map;

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

bool is_diff(routing_table table1,routing_table table2)
{
    node t1node1 = table1.node1;
    node t1node2 = table1.node2;
    node t2node1 = table2.node1;
    node t2node2 = table2.node2;
    return (t1node1.cost != t2node1.cost || t1node2.cost != t2node2.cost);
}

void print_diff(routing_table table1,routing_table table2)
{
    if(is_diff(table1,table2))
    {
        cout<<"Before Update Table"<<endl;
        print_table(table1);
        cout<<"After Update Table"<<endl;
        print_table(table2);
    }else{
        cout<<"Node "<<table1.name<<" routing table not changed"<<endl;
    }
    node t1node1 = table1.node1;
    node t1node2 = table1.node2;
    node t2node1 = table2.node1;
    node t2node2 = table2.node2;
    if(t1node1.cost != t2node1.cost)
    {
        cout<<"Node "<<t1node1.name<<" cost changed from:"<<t1node1.cost<<" to "<<t2node1.cost<<endl;
    }
    if(t1node2.cost != t2node2.cost)
    {
        cout<<"Node "<<t2node1.name<<" cost changed from:"<<t1node2.cost<<" to "<<t2node2.cost;
    }
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

void update_cost(routing_table *table,const char* name,int cost)
{
    int found = 0;
    node node1 = table->node1;
    node node2 = table->node2;
    if(strcmp(node1.name,name) == 0)
    {
        table->node1.cost = cost;
        found = 1;
    }
    if(strcmp(node2.name,name) == 0)
    {
        table->node2.cost = cost;
        found = 1;
    }
    if(!found)
    {
        cout<<"Node not found"<<endl;
    }
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
        string name = string(recv.name);
        if(global_table_map.count(name) > 0)
        {
            routing_table old = global_table_map[name];
            print_diff(old,recv);
        }
        global_table_map[name] = recv;
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
        cout<<"Input command(load,send,bye,myroutingtable,update):"<<endl;
        string cmd;
        getline(cin,cmd);
        if(cmd == "bye")
            break;
        if(cmd == "send")
        {
            sendto(table.node1.ip,table.node1.port,table);
            sendto(table.node2.ip,table.node2.port,table);
            cout<<"Send routing table finished!"<<endl;

        }else if(cmd == "myroutingtable")
        {
            print_table(table);
        }else if(cmd == "load")
        {
            table = read(argv[1]);
            cout<<"Reload config finished!"<<endl;
        }else if(cmd.find("update") == 0){
            vector<string> cmds = split(cmd.c_str()," ");
            if(cmds.size() != 3)
            {
                cout<<"Useage:update <node name> <cost>"<<endl;
                continue;
            }
            string name = cmds.at(1);
            string cost = cmds.at(2);
            int intCost = atoi(cost.c_str());
            update_cost(&table,name.c_str(),intCost);
            sendto(table.node1.ip,table.node1.port,table);
            sendto(table.node2.ip,table.node2.port,table);
        }else{
            cout<<"Invalid command!"<<endl;
        }
    }
}
