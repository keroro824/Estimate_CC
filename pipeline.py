import argparse
import csv
import random
import os.path
import pickle
import ngram
import datetime
import unionfind
from sklearn import linear_model, ensemble, svm


def preprocess(inputf, standard):
	raw = {}
	#read raw data
	Allpair = {}
	with open(standard, 'rb') as pairs:
		pairs.readline()
		reader = csv.reader(pairs, delimiter=';')
		i=1
		for row in reader:
			if row[-1] in Allpair:
				Allpair[row[-1]].append(i)
			else:
				Allpair[row[-1]] = [i]
			raw[i] = row
			i+=1
	Total = len(raw)

	#save all real pairs
	goldPairs = []
	for cluster in Allpair:
		if len(Allpair[cluster])>1:
			values = Allpair[cluster]
			for i in range(len(values)):
				for j in range(i+1, len(values)):
					goldPairs.append((values[i], values[j]))

	#read candidate pairs
	candidates = []
	scores = []
	with open(inputf, 'rb') as candidate:
		reader = csv.reader(candidate, delimiter=' ')
		reader.next()
		for row in reader:
			candidates.append((int(row[0]),int(row[1])))
			datapoint = cal_score(int(row[0]), int(row[1]), raw, len(raw[1])-1)
			scores.append(datapoint)

	return  candidates, Allpair, Total, raw, goldPairs, scores


def estimate(candidates, Total, raw, goldPairs, trainsize, scores, flag):
	#split train and test
	posnum = int(float(trainsize)*len(goldPairs))
	negnum = posnum

	poslist = []
	poslabels = []
	pospair = []

	neglist = []
	neglabels = []
	negpair = []

	trainlist = []
	trainlabels = []
	testlist = []
	testlabels = []

	train_pair = []
	test_pair = []

	randomresultlist = []
	randomresultlabels = []
	random_pair = []

	hashinglist = []
	hashinglabels = []
	hashing_pair = []


	randomlist = {}
	random.shuffle(goldPairs)
	for i in range(posnum):
		datapoint = cal_score(goldPairs[i][0], goldPairs[i][1], raw, len(raw[1])-1)
		poslist.append(datapoint[1:])
		poslabels.append(datapoint[0])
		pospair.append((goldPairs[i][0], goldPairs[i][1]))

	count = 0
	for i in range(len(raw)**2):
		if count==negnum:
			break
		a = random.randint(1, len(raw)-1)
		b = random.randint(1, len(raw)-1)
		amax = max(a, b)
		bmin = min(a, b)
		if raw[a][-1]!=raw[b][-1]:
			count+=1
			datapoint = cal_score(bmin, amax, raw, len(raw[1])-1)
			neglist.append(datapoint[1:])
			neglabels.append(datapoint[0])
			negpair.append((bmin, amax))

	trainlist = poslist[:posnum/2]+neglist[:negnum/2]
	trainlabels = poslabels[:posnum/2]+neglabels[:negnum/2]
	train_pair = pospair[:posnum/2]+negpair[:negnum/2]

	testlist = poslist[posnum/2:]+neglist[negnum/2:]
	testlabels = poslabels[posnum/2:]+neglabels[negnum/2:]
	test_pair = pospair[posnum/2:]+negpair[negnum/2:]


	for i in range(len(candidates)):
		datapoint = scores[i]
		hashinglist.append(datapoint[1:])
		hashinglabels.append(datapoint[0])
		hashing_pair.append(candidates[i])

	for i in range(len(candidates)):
		a = random.randint(1, len(raw)-1)
		b = random.randint(1, len(raw)-1)
		if (a==b):
			b = random.randint(1, len(raw)-1)
		amax = max(a, b)
		bmin = min(a, b)
		datapoint = cal_score(bmin, amax, raw, len(raw[1])-1)
		randomresultlist.append(datapoint[1:])
		randomresultlabels.append(datapoint[0])
		random_pair.append((bmin, amax))

	#train svm
	svmt = svm.SVC(C=100)
	svmt.fit(trainlist, trainlabels)

	#test on testing data
	testresultlist = svmt.predict(trainlist+testlist)

	#test on random selection
	randomselection = svmt.predict(randomresultlist)
	Predict_pairs_random = sum(randomselection)

	#test on hashing selection
	hashingselection = svmt.predict(hashinglist)
	Predict_pairs_hashing = sum(hashingselection)

	random_recall = calculate_pr(randomselection,testresultlist, trainlabels+testlabels, train_pair+test_pair, random_pair, raw)
	if random_recall == 'inf':
		estimate_random = random_recall
	else:
		estimate_random = probability(randomselection, random_recall, random_pair, raw, int(flag))

	hashing_recall = calculate_pr( hashingselection, testresultlist,trainlabels+testlabels, train_pair+test_pair, hashing_pair, raw)
	estimate_hashing = probability(hashingselection, hashing_recall, hashing_pair, raw, int(flag))

	return estimate_random, estimate_hashing


def cal_score(i, j, raw, length):
	result = [int(raw[i][-1]==raw[j][-1])]
	candidate1 = raw[i]
	candidate2 = raw[j]
	for index in range(length):
		score = ngram.NGram.compare(candidate1[index], candidate2[index], N=3)
		result.append(score)
	return result


def union_find(lis, n):
	u = unionfind.unionfind(n+1)
	for pair in lis:
		u.unite(pair[0], pair[1])
	return u.sizes()


def probability(result, p, c_pair, raw, flag):
	cluster = {}
	neighbors = {}
	checklist = []
	j = 0

	if flag:
		for i in range(len(c_pair)):
			if result[i]==1:
				checklist.append(c_pair[i])
			j+=1
	else:
		for i in range(len(c_pair)):
			if raw[c_pair[i][0]][-1]==raw[c_pair[i][1]][-1]:
				checklist.append(c_pair[i])
			j+=1

	

	neighbors = union_find(checklist, len(raw))
	
	n2 = 0
	n3 = 0
	n4 = 0
	nn = 0
	track = 0
	#print "long"
	for neighbor in neighbors:
		
		if neighbors[neighbor]==1:
			track+=1
		elif neighbors[neighbor]==2:
			n2+=1
		elif neighbors[neighbor]==3:
			n3+=1
		else:
			nn+=1
			
#	n4o = 1.0*n4/(1-((1-p)**3)*(p**3)*4-((1-p)**4)*(p**2)*15- ((1-p)**5)*(p)*6)
	n1 = track-1
	n3o = 1.0*n3/(1 - 3*(1-p)**2*p - (1-p)**3)
	n2o = 1.0*(n2 - n3o*(3*(1-p)**2*p))/p
	n1o = n1 - 2*n2o*(1-p) - 3*n3o*(1-p)**3 - 3*n3o*p*(1-p)**2
	return n1o+n3o+n2o+nn


def calculate_pr(resultlist, testresultlist, labels, test_pair, c_pair, raw):
	P = sum(labels)
	c_pair_dic = {}
	indx = 0
	for elem in c_pair:

		c_pair_dic[elem] = indx
		indx+=1

	a=0
	for i in range(len(labels)):
		if labels[i]==1:
			if test_pair[i] in c_pair_dic:
				if resultlist[c_pair_dic[test_pair[i]]]==1:
					a+=1
	# print "hashing recall", a*1.0/P

	if a==0:
		return 'inf'
	else:
		return (a*1.0/P)



def main():
	parser = argparse.ArgumentParser(description='Process.')
	parser.add_argument('input', help='file which has all candidate pairs')
	parser.add_argument('output', help='output file')
	parser.add_argument('goldstan', help='file which has raw data with all ground truth labels')
	parser.add_argument('--trainsize', default='0.1', help='percentage of total pairs to use in training')
	parser.add_argument('--iter', default='100', help='iterations')
	parser.add_argument('--flag', default='1', help='If using full labels 1, if using SVM 0')
	args = parser.parse_args()
	#process input candidate pairs stage
	candidates, Allpair, Total, raw, goldPairs, scores = preprocess(args.input, args.goldstan)

	#SVM stage
	with open(args.output, 'a+') as write:
		writer = csv.writer(write, delimiter=' ')
		for i in range(int(args.iter)):
			estimate_random, estimate_hashing = estimate(candidates, Total, raw, goldPairs, args.trainsize, scores, args.flag)
			writer.writerow([estimate_random, estimate_hashing])
			print estimate_random, estimate_hashing


if __name__ == "__main__":
	main()
