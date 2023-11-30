def justTime(string): 
    return int(string) 
def extract_value(words): 
   for i,w in enumerate(words):
            if w == "rx_count:": 
                return words[i+1]

def extract_node(source): 
    for i,w in enumerate(words):
            if w == "Pckt": 
                return words[i+1]  
            if w == "Tx_try":   
                return words[i+1] 
            if w == "increased_Bandwidth": 
                return words[i+1] 

i = 1  
max = 10
size = 10
for i in range(1,max+1):  
  file_name = f"N{size}log{i}"  

  path_to_file =   f"/Users/guilhermeaguiar/Desktop/DualRadioCodeDrip/Logs/{size}N/" + file_name + ".txt"
  print(path_to_file) 
  with open(path_to_file) as tf: 
    for line in tf:  
       words = line.split() 
       if "rx_count:" in words: 
          print(extract_value(words))  
  
