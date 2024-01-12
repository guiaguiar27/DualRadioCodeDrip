import re
import scipy.stats
import numpy as np
import sys   
import statistics

# to calculate confidence interval   
def conf(data): 
        # Calculate sample mean and standard deviation
    mean = np.mean(data)
    std_dev = np.std(data, ddof=1)  # ddof=1 for sample standard deviation

    # Set the confidence level and calculate the confidence interval
    confidence_level = 0.95
    alpha = 1 - confidence_level
    z_score = scipy.stats.norm.ppf(1 - alpha / 2)  # Z-score for a normal distribution

    # Calculate the margin of error
    margin_of_error = z_score * (std_dev / np.sqrt(len(data)))
 
    return margin_of_error
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

def extract_value_tx(words): 
   for i,w in enumerate(words): 
            if w == "tx:":  
                return words[i+1]
def get_number(source):  
    #print(source)
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

def extract_from_list(listRef, index): 
    for i in range(len(listRef)): 
        if i == index-1: 
            return listRef[i] 

def sum_list(listRef): 
    sum = 0 
    for i in listRef: 
        sum += i 
    return sum 

def find_line_with_string(file_path, target_string):
    sum = 0 
    with open(file_path, 'r') as file: 

        for line_number, line in enumerate(file, 1):
            if target_string in line: 
                sum +=1  
        return sum




i = 1  
max = 10
packet_size = 125 
thr_list = [] 
pdr_list = []
thr_list_size = max  
flag_test = 1
if flag_test == 0: 
    flag  = "o" 
else: 
    flag =  ""
sizes = [10,20,30,40,50]
for size in  sizes: 
    for i in range(1,max+1): 
        my_list = [] 
        list_size = size
        my_list = [0] * list_size 
        time_index = 0
        # get the packet 

        file_name = f"{size}Nlog{i}"  
        path_to_file =   f"/Users/guilhermeaguiar/Desktop/DualRadioCodeDrip/Logs/{size}{flag}/" + file_name + ".txt" 
        #print(path_to_file)

        num_packet_sent = 0 

        with open(path_to_file) as tf: 
            for line in tf:   
            
                num_packet_recpt = 0    
                words = line.split() 
               
                if "tx:" in words:  
                    num_packet_sent += int(extract_value_tx(words))*2 
                    #print(num_packet_sent)
                if "rx_count:" in words:   
                    #extract_node(words)  
                    node = get_number(words) 
                    if time_index == 0: 
                        start_time = int(words[0]) 
                        time_index +=1 
                    final_time = int(words[0])

                    if(len(extract_value(words))  > 4):  

                        num_packet_recpt =  extract_from_list(my_list,int(node)) + 2   
                        #print(num_packet_recpt)  
                        
                    elif(int(extract_value(words)) > num_packet_recpt): 
                        num_packet_recpt = int(extract_value(words)) 
                        #print(num_packet_recpt) 
                    
                    add_list_nodes(my_list,num_packet_recpt,int(node))
                    #print(my_list[:])  
                    #print(sum_list(my_list)) 

        
        if(flag_test == 0):
            recpt_to_pdr  = find_line_with_string(path_to_file, " rx ") 
        else: 
            recpt_to_pdr = sum(my_list)    
         
        pdr = (recpt_to_pdr/num_packet_sent) * 100  
        if pdr>100:  pdr = 100

        pdr_list.append(pdr)  
   
        thr = (recpt_to_pdr * packet_size)/((final_time - start_time)/1000) 
        #print(thr)
        thr_list.append(thr)          
 
    #print(thr_list[:]) 
    #print(confident_interval_data(thr_list,0.95)) 
    #print(thr_list[:])
    #print(average(thr_list))
    #print(average(pdr_list)) 
    #print(conf(thr_list)) 
    print(conf(pdr_list))