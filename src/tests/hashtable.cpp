/* -*-C++-*-
   (c) Copyright 2003-2005, Hewlett-Packard Development Company, LP

   See the file named COPYING for license details
*/

/** @file
    HashTable test program
*/

#include <stdio.h>

#include <Lintel/HashTable.hpp>
#include <Lintel/HashMap.hpp>
#include <Lintel/AssertBoost.hpp>
#include <Lintel/MersenneTwisterRandom.hpp>

using namespace std;
using boost::format;

HashMap<string,string> test;

class intHash {
public:
    unsigned int operator()(const int k) const {
	return k;
    }
};

class collisionHash {
public:
    unsigned int operator()(const int) const {
	return 0;
    }
};

class intEqual {
public:
    bool operator()(const int a, const int b) const {
	return a == b;
    }
};

void stringHTTests() {
    HashMap<string, unsigned> test;

    for(unsigned i = 0; i < 1000; ++i) {
	char buf[30];
	snprintf(buf,30,"%d", i);
	
	test[string(buf)] = i;
    }

    for(unsigned i = 0; i < 1000; ++i) {
	char buf[30];
	snprintf(buf,30,"%d", i);
	
	SINVARIANT(test[string(buf)] == i);
    }

}

typedef HashTable<int,intHash,intEqual> inttable;

void nonconstHTTest(inttable &table, int maxi) {
    vector<bool> found;
    found.resize(maxi);
    inttable::iterator i = table.begin();
    SINVARIANT(i.atStartOfChain());
    uint32_t start_chain_count = 0, end_chain_count = 0;
    for(;i != table.end(); ++i) {
	if (i.atStartOfChain()) {
	    ++start_chain_count;
	}
	if (i.atEndOfChain()) {
	    ++end_chain_count;
	}
	
	SINVARIANT(*i < maxi);
	SINVARIANT(found[*i] == false);
	found[*i] = true;
    }
    SINVARIANT(i.atStartOfChain() && i.atEndOfChain());
    SINVARIANT(start_chain_count > 0 && start_chain_count == end_chain_count);

    for(int i=0; i<maxi; ++i) {
	SINVARIANT(found[i]);
    }
}

void constHTTest(const inttable &table, int maxi) {
    vector<bool> found;
    found.resize(maxi);
    for(inttable::const_iterator i = table.begin();
	i != table.end(); ++i) {
	SINVARIANT(*i < maxi);
	SINVARIANT(found[*i] == false);
	found[*i] = true;
    }

    inttable::const_iterator j = table.find(5);
    j = table.find(6);

    for(int i=0; i<maxi; ++i) {
	SINVARIANT(found[i]);
    }
}

void collisionTest() {
    printf("test collision add/remove\n");
    
    HashTable<int,collisionHash,intEqual> collisiontable;
    for(int i=0;i<100;i++) {
	collisiontable.add(i);
    }
    for(int i=0;i<100;i+=2) {
	collisiontable.remove(i);
	size_t count = 0;
	for (HashTable<int,collisionHash,intEqual>::iterator i = collisiontable.begin();
	     i != collisiontable.end();i++) {
	    count++;
	}
	INVARIANT(count == collisiontable.size(),
		  boost::format("?! %d %d") % count % collisiontable.size());
    }

    // multiple adds of the same key are *supposed* to add multiple times.
    collisiontable.clear();
    SINVARIANT(collisiontable.size() == 0);
    for(int i=0;i<100;i++) {
	collisiontable.add(i);
	collisiontable.add(i);
    }
    SINVARIANT(collisiontable.size() == 200);

    // remove the keys (each twice)
    for(int i=0;i<100;i++) {
	collisiontable.remove(i, true);
	collisiontable.remove(i, true);
    }
    SINVARIANT(collisiontable.size() == 0);

    // an efficient way to do add/replace
    collisiontable.clear();
    SINVARIANT(collisiontable.size() == 0);
    for(int i=0;i<100;i++) {
	bool replaced;
	collisiontable.addOrReplace(i, replaced);
	SINVARIANT(replaced == false);
	collisiontable.addOrReplace(i, replaced);
	SINVARIANT(replaced == true);
    }
    SINVARIANT(collisiontable.size() == 100);

    // remove, add, remove the keys
    for(int i=0;i<100;i++) {
	collisiontable.remove(i, true);
	collisiontable.add(i);
	collisiontable.remove(i);
    }
    SINVARIANT(collisiontable.size() == 0);
}

void generalTest() {

    int maxi = 20000;
    inttable table;
    
    printf("test adding...\n");
    for(int i=0;i<maxi;i++) {
	SINVARIANT(table.size() == static_cast<size_t>(i));
	table.add(i);
    }
    
    nonconstHTTest(table, maxi);
    constHTTest(table, maxi);

    for(int i=0;i<maxi;i++) {
	SINVARIANT(table.lookup(i) != NULL);
	SINVARIANT(table.find(i) != table.end());
	SINVARIANT(*table.lookup(i) == i);
	SINVARIANT(*table.find(i) == i);
    }
    for(inttable::iterator i = table.begin();i != table.end();i++) {
	SINVARIANT(*i >= 0 && *i < maxi);
	SINVARIANT(table.find(*i) == i);
    }
    table.add(777777);
    table.remove(777777); // have something removed when copying
    printf("test copying...\n");
    inttable table2;
    table2 = table;
    SINVARIANT(table2.size() == table.size());
    for(int i=0;i<maxi;i++) {
	SINVARIANT(table2.lookup(i) != NULL);
	SINVARIANT(*table2.lookup(i) == i);
    }
    printf("test assigning...\n");
    inttable table3(table2);
    for(int i=0;i<maxi;i++) {
	SINVARIANT(table3.lookup(i) != NULL);
	SINVARIANT(*table3.lookup(i) == i);
    }

    printf("test removing...\n");
    for(int i=0;i<maxi/2;i++) {
	SINVARIANT(table.lookup(i) != NULL);
	SINVARIANT(*table.lookup(i) == i);
	SINVARIANT((int)table.size() == maxi-i);
	table.remove(i);
    }
    for(int i=maxi/2;i<maxi;i++) {
	SINVARIANT(table.lookup(i) != NULL);
	SINVARIANT(*table.lookup(i) == i);
    }
    for(inttable::iterator i = table.begin();i != table.end();i++) {
	SINVARIANT(*i >= maxi/2 && *i < maxi);
    }
    printf("test complete remove...\n");
    for(int i=maxi/2;i<maxi;i++) {
	SINVARIANT((int)table.size() == maxi-i);
	table.remove(i);
    }
    SINVARIANT(table.size() == 0);
    table.clear();
    SINVARIANT(table.size() == 0);
    printf("test removing strange subset...\n");
    for(int i=0;i<10*maxi;i++) {
	table.add(i);
    }
    for(int i=0;i<10*maxi;i++) {
	if ((i % 17) > 10) {
	    table.remove(i);
	}
    }
    for(int i=0;i<10*maxi;i++) {
	const int *x = table.lookup(i);
	if ((i % 17) <= 10) {
	    SINVARIANT(x != NULL && *x == i);
	} else {
	    SINVARIANT(x == NULL);
	    SINVARIANT(table.find(i) == table.end());
	}
    }

    table.clear();
    SINVARIANT(table.size() == 0);
    table.remove(5,false);

    // similar to addOrReplace above, but without 
    // the forced collisions
    table.clear();
    for (int i = 0; i < 1000; i++) {
	bool replaced;
	table.addOrReplace(i, replaced);
	SINVARIANT(replaced == false);
    }
    for (int i = 0; i < 1000; i++) {
	bool replaced;
	table.addOrReplace(i, replaced);
	SINVARIANT(replaced == true);
    }
}

void eraseTest() {
    MersenneTwisterRandom rng;

    cout << format("erase test using seed %d\n") % rng.seedUsed();
    vector<int32_t> ents;
    inttable table;

    static const uint32_t nents = 1000;

    for (uint32_t i = 0; i < nents; ++i) {
	uint32_t v = rng.randInt();
	ents.push_back(v);
	table.add(v);
    }

    MT_random_shuffle(ents.begin(), ents.end(), rng);
    
    for (uint32_t i = 0; i < nents; ++i) {
	SINVARIANT(table.size() == nents - i);
	inttable::iterator e = table.find(ents[i]);
	SINVARIANT(e != table.end());
	table.erase(e);
	for(uint32_t j = i + 1; j < nents; ++j) {
	    INVARIANT(table.lookup(ents[j]) != NULL, format("%d %d") % i % j);
	}
    }
    SINVARIANT(table.empty() && table.size() == 0);
    cout << "erase test passed.\n";
}

int main(int argc, char **) {
    SINVARIANT(argc == 1);

    collisionTest();
    generalTest();
    stringHTTests();
    eraseTest();

    printf("success.\n");
}

