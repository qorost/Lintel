#!/bin/sh
if [ "$1" = "-count" ]; then
    set -e
    F=`cat $LOCKFILE-count`
    perl -e 'select(undef,undef,undef,0.25)'
    echo `expr $F + 1` >$LOCKFILE-count
    exit 0
fi

if [ "$1" = "-bglock" ]; then
    perl $builddir/lintel-flock --filename=$LOCKFILE --callpid=$$
    sleep 2
    echo "Exiting..."
    exit 0
fi

if [ $# != 3 ]; then
    echo "Usage: $0 <srcdir> <builddir> <installdir>"
    exit 1
fi

srcdir=$1
builddir=$2
installdir=$3
[ ! -z "$TMPDIR" ] || TMPDIR=/tmp
LOCKFILE=`mktemp $TMPDIR/lintel-flock.XXXXXXXX`
trap "rm $LOCKFILE*" 0
PERL5LIB=$srcdir:$installdir/share/perl5:$PERL5LIB

export srcdir builddir installdir LOCKFILE PERL5LIB

verify_unlocked() {
    V=`$builddir/lintel-flock --filename=$LOCKFILE --command=true --waittime=0`
    if [ $? != 0 ]; then
	echo "$LOCKFILE unexpectedly locked; got '$V'"
	exit 1
    fi
}

is_unlocked() {
    TMP=`verify_unlocked`
    return $?
}

verify_locked() {
    wait_time=$1
    [ -z "$wait_time" ] && wait_time=0
    V=`$builddir/lintel-flock --filename=$LOCKFILE --command=true --waittime=$wait_time`
    if [ $? = 0 ]; then
	ps -efl | grep lintel-flock
	echo "$LOCKFILE unexpectedly unlocked, wait $wait_time at `date`: got '$V'"
	exit 1
    fi
    if [ "$V" != "Unable to get lock, not running command" ]; then
	echo "ERROR: locked but successful exit? ($V)"
	exit 1
    fi
}
    
is_locked() {
    TMP=`verify_locked`
    return $?
}

### Blocked lock test

echo "---- Blocked lock test ----"
verify_unlocked
echo "Starting sleep `date`..."
$builddir/lintel-flock --filename=$LOCKFILE --command="sleep 10" &
SLEEP_PID=$!
sleep 2
echo "Starting lock timeout at `date`..."
verify_locked 2
echo "lock timeout succeeded"
kill -TERM $SLEEP_PID
wait
echo "Done with blocked lock test"

### Parallel test

echo "---- Parallel test ----"
verify_unlocked
echo "Starting parallel test ($0)"
echo 0 >$LOCKFILE-count
for i in 1 2 3 4 5 6 7 8 9 10; do
    $builddir/lintel-flock --filename=$LOCKFILE -- $0 -count $LOCKFILE-count &
done
wait
if [ "`cat $LOCKFILE-count`" = "10" ]; then
    echo "Success on parallel test"
else
    echo "Failed parallel test `cat $LOCKFILE-count` != 10"
    exit 1
fi

rm $LOCKFILE-count

### Scripting test 1

echo "---- Simple script test ----"
verify_unlocked

$0 -bglock &
while is_unlocked; do
    perl -e 'select(undef,undef,undef,0.1);'
done
verify_locked
sleep 2
tries=0
while is_locked; do
    tries=`expr $tries + 1`
    # 12 ~= 3 more seconds for process to finish sleep and exit
    if [ $tries -gt 12 ]; then
        echo "Too many tries waiting for unlock"
        exit 1
    fi
    perl -e 'select(undef,undef,undef,0.25);'
done
verify_unlocked

### Scripting test 2

echo "---- Interchange test ----"
verify_unlocked
[ -f $LOCKFILE-locked ] && exit 1

(
    for i in 1 2 3; do
	echo "  a-lock-$i"
	unlock=`$builddir/lintel-flock --filename=$LOCKFILE --callpid=$$`
	case $unlock in 
	    success:*) : ;;
            *) echo "?"; exit 1 ;;
        esac

	echo "  a-wait-$i"
        while [ ! -f $LOCKFILE-locked-$i ]; do
	    perl -e 'select(undef,undef,undef,0.1);'
	done

	# other side has seen us locked.

	echo "  a-cleanup-$i"
	[ -f $LOCKFILE-locked-$i ] || exit 1
	rm $LOCKFILE-locked-$i
	
	$builddir/lintel-flock --unlock="$unlock"

	while [ ! -f $LOCKFILE-unlocked-$i ]; do
	    perl -e 'select(undef,undef,undef,0.1);'
	done

	echo "  a-unlocked-$i"
	# other side has seen us unlocked
	[ -f $LOCKFILE-unlocked-$i ] || exit 1
	rm $LOCKFILE-unlocked-$i
    done
    touch $LOCKFILE-success-a
) &

(
    for i in 1 2 3; do
	echo "  b-wait-$i"
	waiting=true
	while $waiting; do
	    unlock=`$builddir/lintel-flock --filename=$LOCKFILE --callpid=$$ --waittime=1`
	    case $unlock in 
		success:*) $builddir/lintel-flock --unlock="$unlock" ;;
	        timeout) waiting=false ;;
		*) echo "?"; exit 1 ;;
	    esac
        done

	# seen them locked.

	echo "  b-locked-$i"
	touch $LOCKFILE-locked-$i
	
	echo "  b-wait2-$i"
	$builddir/lintel-flock --filename=$LOCKFILE --command=true
	[ $? = 0 ] || exit 1
	
	echo "  b-unlocked-$i"
	verify_unlocked
	touch $LOCKFILE-unlocked-$i
    done
    touch $LOCKFILE-success-b
) &

wait
if [ -f $LOCKFILE-success-a -a -f $LOCKFILE-success-b ]; then
    echo "Success on interchange test"
else
    echo "Interchange test failed"
    exit 1
fi

rm $LOCKFILE-success-[ab]

### cleanup

rm $LOCKFILE*
trap '' 0
echo "Success."


