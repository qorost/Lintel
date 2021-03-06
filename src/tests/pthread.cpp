/* -*-C++-*- */
/*
   (c) Copyright 2008, Hewlett-Packard Development Company, LP

   See the file named COPYING for license details
*/

/** @file
    Testing for PThread
*/

#include <iostream>

#include <Lintel/PThread.hpp>

using namespace std;
using boost::format;

void test_isLocked() {
    PThreadMutex mutex;

    mutex.lock();
    SINVARIANT(mutex.trylock() == false);
    mutex.unlock();
    SINVARIANT(mutex.trylock() == true);
    mutex.unlock();

#if LINTEL_PTHREAD_ISLOCKED_AVAILABLE    
    SINVARIANT(mutex.isLocked() == false);
    mutex.lock();
    SINVARIANT(mutex.isLocked() == true);
    mutex.unlock();
    SINVARIANT(mutex.isLocked() == false);
#endif
    cout << "passed test_isLocked();\n";
}

void test_orderingGroup() {
    LockOrderingGroup ordering_group;

    PThreadMutex level1, level2, level3;

    // test valid+invalid scope locked
    try {
	AssertBoostFnBefore(AssertBoostThrowExceptionFn);
	{
	    PThreadScopedLock lock1(level1, ordering_group, 1);
	    PThreadScopedLock lock3(level3, ordering_group, 3);

	    PThreadScopedLock lock2(level2, ordering_group, 2);
	}
	AssertBoostClearFns();
	FATAL_ERROR("?");
    } catch (AssertBoostException &e) {
	AssertBoostClearFns();
	INVARIANT(e.msg == "lock scope down failed: 3 >= 2",
		  format("unexpected error message '%s'") % e.msg);
#if LINTEL_PTHREAD_ISLOCKED_AVAILABLE
	SINVARIANT(!level1.isLocked());
	SINVARIANT(!level2.isLocked());
	SINVARIANT(!level3.isLocked());
#endif
    }

    SINVARIANT(ordering_group.curLevel() == 0);

    // test valid ScopedUnlock
    { 
	PThreadScopedLock lock3(level3, ordering_group, 3);
	PThreadScopedUnlock unlock3(lock3);
	PThreadScopedLock lock2(level2, ordering_group, 2);
    }
	
    // test invalid ScopedUnlock
    try {
	AssertBoostFnBefore(AssertBoostThrowExceptionFn);

	PThreadScopedLock lock1(level1, ordering_group, 1);
	PThreadScopedLock lock3(level3, ordering_group, 3);
	PThreadScopedUnlock unlock1(lock1);
	AssertBoostClearFns();
	FATAL_ERROR("?");
    } catch (AssertBoostException &e) {
	AssertBoostClearFns();
	INVARIANT(e.msg == "lock scope up failed: 3 != 1",
		  format("unexpected error message '%s'") % e.msg);
#if LINTEL_PTHREAD_ISLOCKED_AVAILABLE
	SINVARIANT(!level1.isLocked());
	SINVARIANT(!level2.isLocked());
	SINVARIANT(!level3.isLocked());
#endif
    }

    {
	PThreadScopedOnlyMutex scoped_only;
	PThreadScopedLock lock_scoped_only(scoped_only, ordering_group, 0.1);
	PThreadScopedUnlock unlock_scoped_only(lock_scoped_only);
    }

    cout << "passed test_orderingGroup();\n";
}

void test_doesNotCompile() {
    PThreadScopedOnlyMutex m;

    PThreadScopedLock foo(m); // this is fine
    // m.lock(); // this is not.
}

int main(int, char **) {
    test_isLocked();
    test_orderingGroup();
    return 0;
}
