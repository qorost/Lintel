/* -*-C++-*-
   (c) Copyright 2008, Hewlett-Packard Development Company, LP

   See the file named COPYING for license details
*/

#ifndef LINTEL_STATS_CUBE_HPP
#define LINTEL_STATS_CUBE_HPP

#include <Lintel/HashTupleStats.hpp>
#include <Lintel/Tuples.hpp>

/** @file
    \brief header file for the StatsCube class
*/

namespace lintel {
    /// \brief Functions for use in StatsCube
    namespace StatsCubeFns {
	inline bool cubeAll() {
	    return true;
	}
    
	inline bool cubeHadAny(bool had_any) {
	    return had_any;
	}
    
	inline void addFullStats(Stats &into, const Stats &val) {
	    into.add(val);
	}
    
	inline void addMean(Stats &into, const Stats &val) {
	    into.add(val.mean());
	}
    }

    /** \brief The cube operator
    
        The cube operator, is described in "Data Cube: A Relational
        Aggregation Operator Generalizing Group-By, Cross-Tab, and
        Sub-Totals," by Jim Gray, Surajit Chaudhuri, Adam Bosworth, Andrew
        Layman, Don Reichart, Murali Venkatrao, Frank Pellow and Hamid
        Pirahesh, published in Data Mining and Knowledge Discovery 1997,
        pages = 29-53.  It is an operation that takes a base table,
        represented by a HashTupleStats, and converts it to a mapping from
        an AnyTuple -> Stats by adding to the cube the powerset of
        anytuples generated by each base tuple.  
    
        We have a number of extentions to the basic cube operator.  We
        have a function (OptionalCubeAnyFn) that can decide on a
        per-entry basis whether or not the cube will be created.  Because
        our base data could be more complex, we also allow the base data to
        be of a different type than the cubed data, for example allowing
        the cube to be over quantiles of the mean values in the base data.
    
        While the cube includes a base_data member value to simplify the
        use of the cube, you are not required to use it as you can store
        the HashTupleStats separately.
    */
    template<class Tuple, class StatsT = Stats, class BaseStatsT = StatsT> class StatsCube {
    public:
	// partial tuple types
	typedef tuples::BitsetAnyTuple<Tuple> MyAny;
	typedef HashMap<MyAny, StatsT *> PartialTupleCubeMap;
	typedef typename PartialTupleCubeMap::iterator PTCMIterator;
	typedef std::vector<typename PartialTupleCubeMap::value_type> PTCMValueVector;
	typedef typename PTCMValueVector::iterator PTCMVVIterator;

	// functions controlling cubing...
	typedef boost::function<StatsT *()> StatsFactoryFn;

	typedef boost::function<void (const Tuple &key, StatsT &value)> PrintBaseFn;
	/// Use key.any[#] and key.data.get<#>() for the pieces.
	typedef boost::function<void (const MyAny &key, StatsT &value)> WalkFn;

	typedef boost::function<bool (bool had_any, const MyAny &)> OptionalCubeFn;
	typedef boost::function<void (StatsT &into, const BaseStatsT &val)> CubeStatsAddFn;
	typedef boost::function<bool (const MyAny &partial)> PruneFn;

	explicit 
	StatsCube(const OptionalCubeFn &fn1 = boost::bind(&StatsCubeFns::cubeHadAny, _1),
		  const CubeStatsAddFn &fn2 = boost::bind(&StatsCubeFns::addFullStats, _1, _2),
		  const StatsFactoryFn &fn3 = boost::bind(&detail::newT<StatsT>),
		  const StatsFactoryFn &fn4 = boost::bind(&detail::newT<BaseStatsT>)) 
	    : base_data(fn4), stats_factory_fn(fn3), optional_cube_partial_fn(fn1), 
	      cube_stats_add_fn(fn2) 
	{ }

	~StatsCube() {
	    clear();
	}

	/// Set the function that controls cubing, normally you would
	/// just set this in the constructor, but this lets you change
	/// it later.
	void setOptionalCubeFn(const OptionalCubeFn &fn2) { 
	    optional_cube_partial_fn = fn2;
	}

	/// Set the function that combines cubed data, normally you
	/// would just set this in the constructor, but this lets you
	/// set it later.
	void setCubeStatsAddFn(const CubeStatsAddFn &fn3) {
	    cube_stats_add_fn = fn3;
	}

	/// Add a value into the base data; later you will call cube
	/// to add the combined data to the cube.  This approach is
	/// more efficient if there are going to be any duplicate
	/// original values in the key space.  If you know there won't
	/// be, you can directly call addToCube, but in that case you
	/// probably want to set the optional cube function to cubeAll
	/// so that the base data entry is not discarded.
	void add(const Tuple &key, double value) {
	    base_data.add(key, value);
	}

	/// Cube the internal base data
	void cube() {
	    cube(base_data);
	}

	/// Walk over all the data and cube each of the entries in
	/// hts.  This function will not create entries in the
	/// cube unless a row containing the values was present in the
	/// base data.  If you want the full cross-product of all the
	/// values that would be zero, use zeroCube.
	void cube(const HashTupleStats<Tuple, StatsT> &hts) {
	    hts.walk(boost::bind(&StatsCube<Tuple, StatsT>::addToCube, this, _1, _2));
	}

	typedef detail::TupleToHashUniqueTuple<Tuple> HUTConvert;
	typedef typename HUTConvert::type HashUniqueTuple;

	/// Cube the data including all of the zero values.  If the
	/// base data is sparse this will generate a much larger cube.
	void zeroCube(bool count_zeros = false) {
	    zeroCube(base_data, count_zeros);
	}

	/// Cube the data in hts including all zero values after
	/// generating the set of unique values for each position in
	/// the tuple.  If the base data is sparse, this will generate
	/// a very big cube.
	void zeroCube(const HashTupleStats<Tuple, StatsT> &hts, bool count_zeros = false) {
	    HashUniqueTuple hut;
	    hts.fillHashUniqueTuple(hut);
	    zeroCube(hts, hut, count_zeros);
	}

	/// Cube the data in hts, including all zero values implied by
	/// hut.  This version of the function lets you prune out
	/// parts of the space that you don't want to cube over by
	/// removing values from hut.  For example, if you clear out
	/// one of the hut tuple entries, it is the equivalent of
	/// requiring that value to be any.
	void zeroCube(const HashTupleStats<Tuple, StatsT> &hts, const HashUniqueTuple &hut,
		      bool count_zeros = false) {
	    hts.walkZeros(boost::bind(&StatsCube<Tuple, StatsT>::addToCube, this, _1, _2), hut,
			  count_zeros);
	}

	/// Add a key, value pair into the cube; normally you would
	/// call add, and separately call cube, but if you have no
	/// duplicates, calling this may be slightly more efficient.
	/// You will probably want to set the optional cube function
	/// to cubeAll so that the base data is not discarded.  Note
	/// that if you use this, then you can't do a zeroCube.
	void addToCube(const Tuple &key, StatsT &value) {
	    MyAny tmp_key(key);

	    cubeAddOne(tmp_key, 0, false, value);
	}

	/// Walk over all the entries in the cube in a pseudo-random
	/// order, calling the walk function for each one.
	void walk(const WalkFn fn) {
	    for(PTCMIterator i = cube_data.begin(); i != cube_data.end(); ++i) {
		fn(i->first, *i->second);
	    }
	}

	/// Walk over all the entries in the cube in a sorted order
	/// where "any" sorts after an actual value, calling the walk
	/// function for each one.  This function uses much more
	/// memory than walk since it has to first sort the list of
	/// keys.
	void walkOrdered(const WalkFn fn) {
	    PTCMValueVector sorted;

	    for(PTCMIterator i = cube_data.begin(); i != cube_data.end(); ++i) {
		sorted.push_back(*i);
	    }
	    sort(sorted.begin(), sorted.end());

	    for(PTCMVVIterator i = sorted.begin(); i != sorted.end(); ++i) {
		fn(i->first, *i->second);
	    }
	}

	/// Get a particular cube entry as specifies by key; create it
	/// if it does not exist.
	Stats &getCubeEntry(const MyAny &key) {
	    Stats * &v = cube_data[key];

	    if (v == NULL) {
		v = stats_factory_fn();
	    }
	    return *v;
	}

	/// Return the size of the cube.
	size_t size() {
	    return cube_data.size();
	}

	/// Prune out entries from the cube; you may also want to call
	/// base_data.prune to prune values out of the base data; the
	/// prune functions are different since the cube operates on
	/// bitset-any tuples and the base data operates on base tuples
	void prune(PruneFn fn) {
	    for(PTCMIterator i = cube_data.begin(); i != cube_data.end(); ) {
		if (fn(i->first)) {
		    delete i->second;
		    cube_data.remove(i->first);
		    i.partialReset();
		} else {
		    ++i;
		}
	    }
	}

	/// Clear out the cube, and by default clear out the base data
	/// as well; optionally skip clearing the base data so that it
	/// could be cubed in multiple ways, or partially pruned and
	/// re-cubed.
	void clear(bool clear_base_data = true) {
	    if (clear_base_data) {
		base_data.clear();
	    }
	    for(PTCMIterator i = cube_data.begin(); i != cube_data.end(); ++i) {
		delete i->second;
	    }
	    cube_data.clear();
	}

	/// Estimate the memory usage of the cube; likely to be
	/// somewhat inaccurate.
	size_t memoryUsage() const {
	    return cube_data.memoryUsage() + sizeof(*this) + cube_data.size() * sizeof(StatsT)
		+ base_data.memoryUsage();
	}

	/// Base data for the cube, public so that it can be pruned
	/// and walked without having to make a function that just
	/// returns it.
	HashTupleStats<Tuple, BaseStatsT> base_data;
    private:
	void cubeAddOne(MyAny &key, size_t pos, bool had_any, const StatsT &value) {
	    if (pos == key.length) {
		if (optional_cube_partial_fn(had_any, key)) {
		    cube_stats_add_fn(getCubeEntry(key), value);
		}
	    } else {
		DEBUG_SINVARIANT(pos < key.length);
		key.any[pos] = false;
		cubeAddOne(key, pos + 1, had_any, value);
		key.any[pos] = true;
		cubeAddOne(key, pos + 1, true, value);
	    }
	}

	StatsFactoryFn stats_factory_fn;
	OptionalCubeFn optional_cube_partial_fn;
	CubeStatsAddFn cube_stats_add_fn;

	PartialTupleCubeMap cube_data;
    };
}

#endif
