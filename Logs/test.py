import re
#import scipy.stats
#import numpy as np
from functools import reduce  
import sys 

# to calculate confidence interval  
def confident_interval_data(X, confidence = 0.95, sigma = -1):
    def S(X): #funcao para calcular o desvio padrao amostral
        s = 0
        for i in range(0,len(X)):
            s = s + (X[i] - np.mean(X))**2
        s = np.sqrt(s/(len(X)-1))
        return s
    n = len(X) # numero de elementos na amostra
    Xs = np.mean(X) # media amostral
    s = S(X) # desvio padrao amostral
    zalpha = abs(scipy.stats.norm.ppf((1 - confidence)/2))
    if(sigma != -1): # se a variancia eh conhecida
        IC1 = Xs - zalpha*sigma/np.sqrt(n)
        IC2 = Xs + zalpha*sigma/np.sqrt(n)
    else: # se a variancia eh desconhecida
        if(n >= 50): # se o tamanho da amostra eh maior do que 50
            # Usa a distribuicao normal
            IC1 = Xs - zalpha*s/np.sqrt(n)
            IC2 = Xs + zalpha*s/np.sqrt(n)
        else: # se o tamanho da amostra eh menor do que 50
            # Usa a distribuicao t de Student
            talpha = scipy.stats.t.ppf((1 + confidence) / 2., n-1)
            IC1 = Xs - talpha*s/np.sqrt(n)
            IC2 = Xs + talpha*s/np.sqrt(n)
    return  talpha*s/np.sqrt(n) 

#avarage 
def average(list): 
    return sum(list)/len(list)
# just add two numbers 
def add(a,b): 
    return a + b  
# get the next word by a word reference     
def next(a,source): 
    for i,w in enumerate(source): 
        if w == a: 
            return source[i+1]  


def justTime(string): 
    return int(string)  


def time_extraction(source):  
   pass

def extract_value(words): 
   for i,w in enumerate(words): 
            if w == "rx_count:": 
                return words[i+1]


def get_number(source):  
    print(source)
    first = str(source[1]) 
    sub = first[3:] 
    return int(sub)

   # print(substring)
def extract_node(source): 
    for i,w in enumerate(words): 
            if w == "rx_count:": 
                #print(words[1]) 
                return words[i-1] 

# adciona todos os pacotes em uma lista para ficar mais facil de ser processado 
def add_list_nodes(listRef, value, index): 
     for i in range(len(listRef)):  
          if i == index -1: 
               listRef[i] = value

def sum_list(listRef): 
    sum = 0 
    for i in listRef: 
        sum += i 
    return sum

i = 1  
max = 2
size = 10   
packet_size = 125 

for i in range(2,max+1):  
    num_packet_recpt = 0 
    my_list = [] 
    list_size = size
    my_list = [0] * list_size 

    # get the packet 

    file_name = f"{size}Nlog{i}"  
    path_to_file =   f"/Users/guilhermeaguiar/Desktop/DualRadioCodeDrip/Logs/{size}/" + file_name + ".txt"
    #print(path_to_file) 
    
    with open(path_to_file) as tf: 
        for line in tf:  
            words = line.split() 
            if "rx_count:" in words:   
                #extract_node(words)  
                node = get_number(words) 
                print(node)

                if(len(extract_value(words)) > 4): 
                    num_packet_recpt += 2 
                elif(int(extract_value(words)) > num_packet_recpt): 
                    num_packet_recpt = int(extract_value(words)) 
                    print(num_packet_recpt) 
                
                add_list_nodes(my_list,num_packet_recpt,int(node))
                print(my_list[:])  
                print(sum_list(my_list))
                
        #print(f"Total num packets:{num_packet_recpt}")

