import sys
import queue
import copy
import time

addrs=[]
#sequence of addresses, for optimal 
seq={}
        #page_num,ref_bit,dirty_bit
page_table=[[0,0,0] for x in range(pow(2,20))]
dirty_bits=[0 * pow(2,20)]
ref_bits=[0 * pow(2,20)]

mode=''
nums_frame=0
page_faults=0
mem_count=0
disk_write=0
#refresh for NRU
r=0

def main():
    global addrs
    global mode
    global nums_frame
    global r

    f_name=sys.argv[-1]
    for x in range(len(sys.argv)):
        if sys.argv[x]=='-n':
            nums_frame=int(sys.argv[x+1])
        elif sys.argv[x]=='-a':
            mode=sys.argv[x+1].lower()
        elif sys.argv[x]=='-r':
            r=int(sys.argv[x+1])
    #print(nums_frame,mode,r,f_name)
    f=open(f_name,'r')
    count=0
    for x in f:
        count+=1
        addr=x.strip('\n').split(' ') 
        addrs.append([addr[0],addr[1]])
        #record the addresses demand sequence in a dict for optimal 
        if addr[0][:5] in seq:
            seq[addr[0][:5]][1].append(count)
        else:
                        #the first index denotes the curr pointer in the call seq
            seq[addr[0][:5]]=[0,[count]]

   

    if mode=='fifo':
        FIFO()
    elif mode=='clock':
        Clock()
    elif mode=='opt':
        Opt()
    elif mode=='nru':
        NRU()

    print('''\nAlgorithm: %s\nNumber of frames:       %d\nTotal memory accesses:  %d
Total page faults:      %d\nTotal writes to disk:   %d''' % (mode,nums_frame,mem_count,page_faults,disk_write))


def FIFO():
    global nums_frame
    global page_faults
    global mem_count
    global disk_write
    #instantiate the page frame
    q=queue.Queue(maxsize=nums_frame)
    #use a set to assist fast searching
    p_mem=set()

    for x in addrs:
        mem_count+=1
        page_num=int(x[0][:5],16)
       
        print(str(x)+': ',end='')

        x[0]=x[0][:5]

        if x[0] not in p_mem:
            page_faults+=1
            if q.full():
                evict=q.get()[0]
                p_mem.remove(evict)
                
                if page_table[int(evict,16)][2]==1:
                    disk_write+=1
                    print('page fault - evict dirty')
                    #reset dirty bit if evict
                    page_table[int(evict,16)][2]=0
                else:
                    a=0
                    print('page fault - evict clean')
            else:
                a=0
                print('page fault - no eviction')
            
            p_mem.add(x[0])  
            q.put(x)
        else:
            a=0
            print('hit')

        if x[1]=='W':
                page_table[page_num][2]=1

def Clock():
    global nums_frame
    global page_faults
    global mem_count
    global disk_write
    #use an indexable list and a pointer to simulate the page frame
    q=[0]*nums_frame
    curr=0
    #use a set to assist fast searching
    p_mem=set()
    isFull=False
    for x in addrs:
        mem_count+=1
        page_num=int(x[0][:5],16)
        print(str(x)+': ',end='')
        page_table[page_num][1]=1
        x[0]=x[0][:5]

        if x[0] not in p_mem:
            page_faults+=1
            if isFull:
                while True:
                    evict=q[curr]
                    evict_num=int(evict,16)
                    #search for the unreferenced page
                    if page_table[evict_num][1]==1:
                        page_table[evict_num][1]=0
                        #move clock hand
                        curr=(curr+1)%nums_frame
                        continue
                    #if found then evict
                    else:
                        p_mem.remove(evict)
                        if page_table[evict_num][2]==1:
                            disk_write+=1
                            print('page fault - evict dirty')
                            #reset dirty bit if evict
                            page_table[evict_num][2]=0
                        else:
                            print('page fault - evict clean')
                            #move clock hand
                        break 
            else:
                print('page fault - no eviction')
                if curr==nums_frame-1:
                    isFull=True
                
           

            #assign page - added page is referenced
            p_mem.add(x[0])
            page_table[page_num][1]=1
            q[curr]=x[0]
            #move the clock hand
            curr=(curr+1)%nums_frame

        else:
            print('hit')
        #set dirty bit to 1 if write
        if x[1]=='W':
                page_table[page_num][2]=1


def Opt():
    global nums_frame
    global page_faults
    global mem_count
    global disk_write
    #use an indexable list and a pointer to simulate the page frame
    q=[0]*nums_frame
    curr=-1
    #use a set to assist fast searching
    p_mem=set()
    isFull=False
    for x in addrs:
        mem_count+=1
        page_num=int(x[0][:5],16)
        print(str(x)+': ',end='')
        page_table[page_num][1]=1
        x[0]=x[0][:5]

        if x[0] not in p_mem:
            page_faults+=1
            if isFull:
                #use time to compare the nearest usage time in the future of each page 
                time=-1
                curr=0
                
                for y in range(nums_frame):
                    evict=q[y]
                    #get the nearest usage time from time trace
                    index=seq[evict][0]
                    tmp_seq=seq[evict][1]

                    #if the time index if at the end of the time traces then
                    #the current page has finished being called, no more call in the future, evict immediately
                    if tmp_seq[-1] < mem_count:
                        curr=y
                        break
                    

                    for z in range(index,len(tmp_seq)):
                        #skip if the call time under the question belongs to the past
                        if tmp_seq[z]<mem_count:
                            continue
                        #compare to the longest time so far
                        elif tmp_seq[z]>time:
                            time=tmp_seq[z]
                            curr=y

                        seq[evict][0]=z
                        break


                  
                    #print("mem_count: "+str(mem_count)+" y: "+str(y)+"\nseq: "+str(seq[evict]))

                evict=q[curr]
                evict_num=int(evict,16)
                p_mem.remove(evict)

                if page_table[evict_num][2]==1:
                    disk_write+=1
                    print('page fault - evict dirty')
                    #reset dirty bit if evict
                    page_table[evict_num][2]=0
                else:
                    print('page fault - evict clean')
                
            else:
                print('page fault - no eviction')
                #keep filling the page frame until full
                curr+=1
                if curr==nums_frame-1:
                    isFull=True
           

            #assign page - added page is referenced
            p_mem.add(x[0])
            page_table[page_num][1]=1
            q[curr]=x[0]

        else:
            print('hit')
        #set dirty bit to 1 if write
        if x[1]=='W':
                page_table[page_num][2]=1


def NRU():
    
    global nums_frame
    global page_faults
    global mem_count
    global disk_write
    global r
    
    #use an indexable list and a pointer to simulate the page frame
    q=[0]*nums_frame
    curr=-1
    #use a set to assist fast searching
    p_mem=set()
    isFull=False
    period=0
    for x in addrs:
        #keep track of the memory access and reset the reference bits once reach the refresh limit
        period+=1
        mem_count+=1
        page_num=int(x[0][:5],16)
        print(str(x)+': ',end='')
        page_table[page_num][1]=1
        x[0]=x[0][:5]

        if x[0] not in p_mem:
            page_faults+=1
            if isFull:
                #use a vairable to keep track of the lowest-level class page to evict
                level=5
                chosen=0
                for y in range(nums_frame):
                    evict=q[y]
                    evict_num=int(evict,16)
                    tmp=0
                    if page_table[evict_num][1]==1:
                        if page_table[evict_num][2]==1:
                            tmp=4
                        else:
                            tmp=3
                    else:
                        if page_table[evict_num][2]==1:
                            tmp=2
                        else:
                            tmp=1
                    
                    if tmp<level:
                        curr=y
                        level=tmp

                

                evict=q[curr]
                p_mem.remove(evict)
                
                if page_table[evict_num][2]==1:
                    disk_write+=1   
                    print('page fault - evict dirty')
                    #reset dirty bit if evict
                    page_table[evict_num][2]=0
                else:
                    print('page fault - evict clean')
            else:
                curr+=1
                print('page fault - no eviction')
                if curr==nums_frame-1:
                    isFull=True

            #assign page - added page is referenced
            p_mem.add(x[0])
            page_table[page_num][1]=1
            q[curr]=x[0]

        else:
            print('hit')
            a=1
        #set dirty bit to 1 if write
        if x[1]=='W':
                page_table[page_num][2]=1

        if period == r:
            period=0
            for y in q:
                if y==0: #not full
                    break
                reset=int(y,16)
                page_table[reset][1]=0

    #print("refresh: "+str(r))


            



def test1():
    global addrs
    global nums_frame
    global page_faults
    global mem_count
    global disk_write

    pf=[]
    dw=[]
    ms=[]
    f_num=[8,16,32,64]
    r_num=[8,32,80,160,200,320,640,1200]
    sys.argv[4]='nru'

    for x in f_num:
        sys.argv[2]=x
        for y in r_num:
            sys.argv[6]=y
            print(sys.argv)
            disk_write=0
            page_faults=0
            mem_count=0
            addrs=[]
            
            main()

            pf.append(page_faults)
            dw.append(disk_write)
            ms.append(mem_count)
            

    c=0
    for x in f_num:
        for y in r_num:
            print('''\nAlgorithm: %s\n Refresh: %d\nNumber of frames:       %d\nTotal memory accesses:%d
                Total page faults:      %d\nTotal writes to disk:   %d''' % ('nru',y,x,ms[c],pf[c],dw[c]))
            c+=1

def test3():

    global addrs
    global nums_frame
    global page_faults
    global mem_count
    global disk_write

    res=[]
    sys.argv[4]='fifo'
    prev=float('inf')
    for x in range(2,101):
        sys.argv[2]=x
        disk_write=0
        page_faults=0
        mem_count=0
        disk_write=0
        page_faults=0
        mem_count=0
        addrs=[]

        print(sys.argv)
        main()
        
        res.append("page frames: "+str(sys.argv[2])+" page faults: "+str(page_faults))
        if page_faults>prev:
            res[x-2]+=' anomaly here'
        prev=page_faults

    for x in res:
        print(x)
    


main()
#test1()
#test3()



