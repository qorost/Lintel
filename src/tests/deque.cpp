/* -*-C++-*- */
/*
   (c) Copyright 2008, Hewlett-Packard Development Company, LP

   See the file named COPYING for license details
*/

/** @file
    test for deque
*/

// TODO: add performance comparison to this test
#include <iostream>
#include <inttypes.h>
#include <deque>

#include <Lintel/Deque.hpp>
#include <Lintel/MersenneTwisterRandom.hpp>
using namespace std;
using boost::format;

class NoDefaultConstructor {
public:
    NoDefaultConstructor(int i) : v(i) { ++ndc_count; }
    NoDefaultConstructor(const NoDefaultConstructor &i) : v(i.v) { ++ndc_count; }
    ~NoDefaultConstructor() { --ndc_count; }

    static int ndc_count;
    int v;

private:
    NoDefaultConstructor operator = (const NoDefaultConstructor &);
};

int NoDefaultConstructor::ndc_count;

void testNoDefaultConstructor() {
    Deque<NoDefaultConstructor> deque;

    deque.reserve(8);

    for(int i = 0; i < 5; ++i) {
	deque.push_back(NoDefaultConstructor(i));
	SINVARIANT(deque.back().v == i);
	SINVARIANT(deque.size() == static_cast<size_t>(i + 1));
	INVARIANT(NoDefaultConstructor::ndc_count == i + 1, format("%d != %d + 1")
		  % NoDefaultConstructor::ndc_count % i);
    }

    for(int i = 0; i < 5; ++i) {
	SINVARIANT(deque.front().v == i);
	deque.pop_front();
	deque.push_back(NoDefaultConstructor(i));
	SINVARIANT(deque.back().v == i);
	SINVARIANT(deque.size() == 5);
	SINVARIANT(NoDefaultConstructor::ndc_count == 5);
    }

    for(int i = 0; i < 5; ++i) {
	SINVARIANT(deque.front().v == i);
	deque.pop_front();
	SINVARIANT(deque.size() == static_cast<size_t>(4 - i));
	SINVARIANT(NoDefaultConstructor::ndc_count == 4 - i);
    }
    SINVARIANT(NoDefaultConstructor::ndc_count == 0);
}
    
void testPushBack() {
    Deque<int> deque;

    SINVARIANT(deque.empty());
    deque.reserve(8);
    SINVARIANT(deque.empty() && deque.capacity() == 8);
    for(int i = 0; i < 5; ++i) {
	deque.push_back(i);
	SINVARIANT(deque.back() == i);
	SINVARIANT(deque.size() == static_cast<size_t>(i + 1));
    }

    for(int i = 0; i < 5; ++i) {
	SINVARIANT(deque.at(i) == i);
    }

    for(int i = 0; i < 5; ++i) {
	SINVARIANT(deque.front() == i);
	deque.pop_front();
	deque.push_back(i);
	SINVARIANT(deque.back() == i);
	SINVARIANT(deque.size() == 5);
    }

    for(int i = 0; i < 5; ++i) {
	SINVARIANT(deque.at(i) == i);
    }

    {
	Deque<int>::iterator i = deque.begin(); 
	int j = 0;
	while(i != deque.end()) {
	    INVARIANT(*i == j, format("%d != %d") % *i % j);
	    ++i;
	    ++j;
	}
    }
	
    vector<int> avec;
    for(int i = 5; i < 10; ++i) {
	avec.push_back(i);
    }
    deque.push_back(avec);
    
    for(int i = 0; i < 10; ++i) {
	SINVARIANT(deque.front() == i);
	deque.pop_front();
    }
    SINVARIANT(deque.empty());
}

void testAssign() {
    Deque<int> a;
    a.push_back(1);
    a.push_back(2);
    a.push_back(3);

    vector<int> b;
    b.assign(a.begin(), a.end());
    SINVARIANT(b.size() == 3);
    for(int32_t i = 0; i < 3; ++i) {
	SINVARIANT(b[i] == i+1);
    }
}

struct Thing {
    Thing() { ++thing_count; }
    ~Thing() { --thing_count; }

    Thing(const Thing &) { ++thing_count; }

    Thing &operator =(const Thing &) {
        FATAL_ERROR("no");
    }

    static uint32_t thing_count;
};

uint32_t Thing::thing_count = 0;

// verify that the destructor is called on pop_front.
void testDestroy() {
    Deque<Thing> things;

    for(uint32_t i = 0; i < 10; ++i) {
        INVARIANT(Thing::thing_count == i, format("%d != %d") % Thing::thing_count % i);
        things.push_back(Thing());
    }
    
    for(uint32_t i = 0; i < 10; ++i) {
        SINVARIANT(Thing::thing_count == (10-i));
        things.pop_front();
    }
    
    SINVARIANT(Thing::thing_count == 0);
}

void performSortingTest(int seed = 0) {
    MersenneTwisterRandom rng(seed);
    deque<int> std_deq;
    Deque<int> lintel_deq;

    // Note: In this test we do not reserve the Deque because std::deque doesn't have such
    // an operation and resize will put values in.

    int n_ops = rng.randInt(100);
    // perform random push_back(70%) and pop_front(30%) operations. These numbers are arbitrary.
    for (int i = 0; i < n_ops; ++i) {
        if (rng.randInt(10) < 3 && !std_deq.empty()) {
            SINVARIANT(!lintel_deq.empty());
            std_deq.pop_front();
            lintel_deq.pop_front();
        } else {
            int rand_num = rng.randInt();
            std_deq.push_back(rand_num);
            lintel_deq.push_back(rand_num);
        }
    }
    sort(std_deq.begin(), std_deq.end());
    sort(lintel_deq.begin(), lintel_deq.end());

    deque<int>::iterator s_it = std_deq.begin();
    Deque<int>::iterator l_it = lintel_deq.begin();

    for ( ; s_it != std_deq.end(); ++s_it, ++l_it) {
        INVARIANT(*s_it == *l_it, format("%d != %d") % *s_it % *l_it);
    }
    SINVARIANT(l_it == lintel_deq.end());
}

void testSorting() {
    // magical seed to test for q_front > q_back and partially filled deque.
    performSortingTest(1090728558);
    for (int i = 0; i < 100; ++i) {
        // test with random seeds
        performSortingTest();
    }
}

void testIteratorOperations() {
    Deque<int> deque;
    MersenneTwisterRandom rng;
    int n_ops = rng.randInt(100);
    for (int i = 0; i < n_ops; ++i) {
        if (rng.randInt(10) < 3 && !deque.empty()) {
            deque.pop_front();
        } else {
            deque.push_back(rng.randInt());
        }
    }

    // Test iterator unary operators
    {
        for (Deque<int>::iterator it = deque.begin(); it != deque.end(); ) {
            Deque<int>::iterator it1 = it;
            Deque<int>::iterator it2 = it;
            SINVARIANT(it1 == it2);
            Deque<int>::iterator it3 = it1++;
            INVARIANT(it3 == it && it3 != it1 && it1 != it2,
                      "return unchanged && return different from iterator && iterator changed");
            Deque<int>::iterator it4 = ++it2;
            INVARIANT(it4 != it && it4 == it1 && it2 == it1,
                      "return changed && return == updated && two updates same");
            it = it4;
        }
    }

    // Test distance operators
    Deque<int>::iterator it_forward = deque.begin();
    Deque<int>::iterator it_backward = deque.end();
    ptrdiff_t dist_from_start = 0; // start can be .begin() or .end()
    ptrdiff_t dist_from_finish = deque.end() - deque.begin(); // finish can be .end() or .begin()
    for (; it_forward != deque.end(); ++dist_from_start, --dist_from_finish) {
        SINVARIANT(it_backward - it_forward == dist_from_finish - dist_from_start);

        SINVARIANT(it_forward + dist_from_finish == deque.end());
        SINVARIANT(it_backward - dist_from_finish == deque.begin());

        SINVARIANT((it_forward < it_backward) == (dist_from_start < dist_from_finish));
        SINVARIANT((it_forward <= it_backward) == (dist_from_start <= dist_from_finish));

        SINVARIANT((it_forward > it_backward) == (dist_from_start > dist_from_finish));
        SINVARIANT((it_forward >= it_backward) == (dist_from_start >= dist_from_finish));

        Deque<int>::iterator temp_a(it_forward);
        Deque<int>::iterator temp_b;
        temp_b = it_backward;
        SINVARIANT(temp_b - temp_a == dist_from_finish - dist_from_start);

        temp_a += dist_from_finish;
        SINVARIANT(temp_a == deque.end());
        temp_b -= dist_from_finish;
        SINVARIANT(temp_b == deque.begin());
        if (rng.randBool()) { // Exercise both variants of the increment/decrement operators
            ++it_forward;
            --it_backward;
        } else {
            it_forward++;
            it_backward--;
        }
    }
    SINVARIANT(it_backward == deque.begin());
    SINVARIANT(static_cast<size_t>(dist_from_start) == deque.size());
    SINVARIANT(dist_from_finish == 0);
}

int main(int, char **) {
    testPushBack();
    testNoDefaultConstructor();
    testAssign();
    testDestroy();
    testSorting();
    testIteratorOperations();
    cout << "deque tests passed.\n";
}

