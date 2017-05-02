# -*- coding: utf-8 -*-
import random
import os

def generate_normal_data(file_name,max_key,total):
	key_list=[]
	# create data
	count=0
	while(count<total):
		cur_key=random.randint(1,max_key)
		key_list.append(cur_key)
		count=count+1
	# insert data into file
	fh=open(file_name,'w+')
	# fh.write("N %d\n" % total)
	for key in key_list:
		fh.write("i %d\n" % key)
	# fh.write("F 0\n")
	fh.close()

def get_operation():
	seed=random.random()
	if (0<=seed<0.3):
		operation='i'
	elif(0.3<=seed<0.6):
		operation='r'
	else:
		operation='s'
	return operation

def generate_opeartion(file_name,max_key,total):
	count=0
	fh=open(file_name,'a+')
	while(count<total):
		count=count+1
		cur_key=random.randint(1,max_key)
		operation=get_operation()
		if(operation == 'r'):
			key=random.randint(1,max_key)
			if(cur_key <= key):
				(range_left,range_right)=(cur_key,key)
			else:
				(range_left,range_right)=(key,cur_key)
			fh.write("r %d %d\n" % (range_left,range_right))
		else:
			fh.write("%c %d\n" % (operation,cur_key))
	fh.write("q\n")
	fh.close


# create single test case !!
generate_normal_data("./testcase",0xfffff,1e+6)
generate_opeartion("./testcase",0xfffff,2e+6)

#os.system('pause')
