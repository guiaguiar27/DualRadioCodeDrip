# code to collect throughput from cooja simulations  
import re
import scipy.stats
import numpy as np
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



def extract_rx_count(file_path):
    try:
        with open(file_path, 'r') as file:
            content = file.read()

            # Using regular expression to find the number after "rx_count"
            match = re.search(r'rx_count\s*:\s*(\d+)', content)

            if match:
                rx_count = match.group(1)
                return int(rx_count)
            else:
                print("No 'rx_count' found in the file.")
                return None

    except FileNotFoundError:
        print(f"File not found: {file_path}")
        return None

# Replace 'your_file.txt' with the actual path to your text file
file_path = 'your_file.txt'
result = extract_rx_count(file_path)

if result is not None:
    print(f"Rx Count: {result}")
# get the number of packets sent by a specif node 
