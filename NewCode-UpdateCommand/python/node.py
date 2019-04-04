import socket
import json
import operator
import sys
import binascii
import struct
import threading
import configparser


def is_diff(obj1,obj2):
    if obj1 == None or obj2 == None:
        return False
    str1 = json.dumps(obj1)
    str2 = json.dumps(obj2)
    return str1 != str2

def print_table(obj):
    if obj == None:
        print(">>>>>> table is empty <<<<<<<<")
        return
    print('>>>> ' + obj['node']['name'] + ' routing table <<<<')
    print('-------------------------------------------------------')
    print('|   destination   |    link cost    |    next hop     |')
    print('|    %-13s|    %-13s|    %-13s|' % (obj['link1']['name'],obj['link1']['cost'],obj['link1']['name']))
    print('|    %-13s|    %-13s|    %-13s|' % (obj['link2']['name'],obj['link2']['cost'],obj['link2']['name']))
    print('-------------------------------------------------------')


def print_diff(obj1,obj2):
    if obj1 == None or obj2 == None:
        return
    if is_diff(obj1,obj2):
        print("Before Update Table")
        print_table(obj1)
        print("After Update Table")
        print_table(obj2)
        
        for k in obj1:
            if k == "node":
                continue
            if obj2[k]['cost'] != obj1[k]['cost']:
                print("Node " + obj2[k]['name'] + " cost changed from " + str(obj1[k]['cost']) + " to " + str(obj2[k]['cost']))
    else:
        print("Node " + obj1['node']['name'] + " routing table not changed")



def listen_thread(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("0.0.0.0", port))
    while True:
        data, addr = s.recvfrom(1024)
        print(data)

def send(str,ip,port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.sendto(bytes(str,'utf8'),(ip,port))

class RecvThread(threading.Thread):
    def __init__(self,port):
        super(RecvThread, self).__init__()
        self.port = port

    def run(self):
        global node_configs
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind(("0.0.0.0", self.port))
        while True:
            data, addr = s.recvfrom(1024)
            print("Recv: ");
            dict = json.loads(str(data,encoding='utf-8'))
            print_table(dict)
            name = dict['node']['name']
            old = None
            if name in node_configs:
                old = node_configs[name]
            node_configs[name] = dict
            print_diff(old,dict)
            print("Input command(load,send,bye):")

class MyParser(configparser.ConfigParser):
    def as_dict(self):
        d = dict(self._sections)
        for k in d:
            d[k] = dict(d[k])
            d[k].pop('__name__', None)
        return d

if len(sys.argv) != 2:
    print("Useage: python " + sys.argv[0] + " <confg file>")
    sys.exit(-1)

def load_ini(file):
    cf = MyParser()
    cf.read(file)
    return cf.as_dict()

config_dict = load_ini(sys.argv[1])
listen_port = int(config_dict['node']['port'])
#run recv thread
t = RecvThread(int(listen_port))
t.setDaemon(True)
t.start()

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

node_configs = {}


def send():
    global s
    #send to link1
    s.sendto(bytes(json.dumps(config_dict),'utf8'),(config_dict['link1']['ip'],int(config_dict['link1']['port'])))
    #send to link2
    s.sendto(bytes(json.dumps(config_dict),'utf8'),(config_dict['link2']['ip'],int(config_dict['link2']['port'])))
    print("Send config finished")


def load(ini):
    config_dict = load_ini(ini)
    print("Load config file finished")

def update_cost(node,cost):
    if not cost.isdigit():
        print("Cost is not number")
        return
    found = False
    for k in config_dict:
        if k == "node":
            continue
        v = config_dict[k]
        tmp_name = v['name']
        if tmp_name.lower() == name:
            v['cost'] = cost
            found = True
    if not found:
        print("Node <" + name + "> not found in table")
    else:
        send()
while True:
    print("Input command(load,send,bye,myroutingtable,update):")
    text = sys.stdin.readline().strip().lower()
    if text == "send":
        send()
    elif text == "load":
        load(sys.argv[1])
    elif text == "bye":
        break
    elif text == "myroutingtable":
        print_table(config_dict)
    elif text.startswith("update"):
        cmds = text.split(" ")
        if len(cmds) != 3:
            print("Update command usage:update <node name> <cost>")
            continue
        name = cmds[1]
        cost = cmds[2]
        update_cost(name,cost)           
    elif text == "print":
        print_table(config_dict)
    else:
        print("Invalid command")


