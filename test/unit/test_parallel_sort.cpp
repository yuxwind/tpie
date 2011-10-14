// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; c-file-style: "stroustrup"; -*-
// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2008, The TPIE development team
// 
// This file is part of TPIE.
// 
// TPIE is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
// 
// TPIE is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with TPIE.  If not, see <http://www.gnu.org/licenses/>

#include "common.h"
#include <tpie/parallel_sort.h>
#include <boost/random/linear_congruential.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <tpie/progress_indicator_arrow.h>
#include <tpie/memory.h>

static bool progress;
static bool stdsort;

using namespace tpie;

template<size_t min_size>
bool basic1(size_t elements = 1024*1024) {
	boost::rand48 prng(42);
	std::vector<int> v1(elements);
	std::vector<int> v2(elements);

	auto_ptr<progress_indicator_arrow> pi;
	auto_ptr<fractional_progress> fp;
	auto_ptr<fractional_subindicator> gen_p;
	auto_ptr<fractional_subindicator> std_p;
	auto_ptr<fractional_subindicator> par_p;
	if (progress) {
		pi.reset(tpie_new<progress_indicator_arrow>("Parallel sort", elements));
		fp.reset(tpie_new<fractional_progress>(pi.get()));

		gen_p.reset(tpie_new<fractional_subindicator>(*fp.get(), "Generate", TPIE_FSI, elements, "Generate"));
		if (stdsort) std_p.reset(tpie_new<fractional_subindicator>(*fp.get(), "std::sort", TPIE_FSI, elements, "std::sort"));
		par_p.reset(tpie_new<fractional_subindicator>(*fp.get(), "parallel_sort", TPIE_FSI, elements, "parallel_sort"));
		fp->init();
	}

	if (progress) gen_p->init(elements);
	for (size_t i = 0; i < elements; ++i) {
		if (progress) gen_p->step();
		if (stdsort)
			v1[i] = v2[i] = prng();
		else
			v1[i] = prng();
	}
	if (progress) gen_p->done();

	parallel_sort_impl<std::vector<int>::iterator, std::less<int>, min_size > s(progress ? par_p.get() : 0);
	s(v2.begin(), v2.end());

	if (stdsort) {
		if (progress) std_p->init(1);
		std::sort(v1.begin(), v1.end());
		if (progress) std_p->done();
	}

	if (progress) fp->done();

	if (stdsort && v1 != v2) {
		std::cerr << "std::sort and parallel_sort disagree" << std::endl;
		return false;
	}
	return true;
}

bool equal_elements() {
	std::vector<int> v1;
	std::vector<int> v2;
	for (size_t i=0; i < 1234567; ++i) {
		v1.push_back(42);
		v2.push_back(42);
	}
	v1.push_back(1);
	v2.push_back(1);
	v1.push_back(64);
	v2.push_back(64);

	boost::posix_time::ptime t1=boost::posix_time::microsec_clock::local_time();
	std::sort(v1.begin(), v1.end());
	boost::posix_time::ptime t2=boost::posix_time::microsec_clock::local_time();
	parallel_sort_impl<std::vector<int>::iterator, std::less<int>, 42> s(0);
	s(v2.begin(), v2.end());
	boost::posix_time::ptime t3=boost::posix_time::microsec_clock::local_time();
	if(v1 != v2) {std::cerr << "Failed" << std::endl; return false;}
	std::cout << "std: " << (t2-t1) << " ours: " << t3-t2 << std::endl;
	if( (t2-t1)*3 < (t3-t2) ) {std::cerr << "Too slow" << std::endl; return false;}
	return true;
}

void stress_test() {
	boost::posix_time::time_duration d;
	{
		std::vector<int> v;
		for(int i = 0; i < 4000000; ++i)
			v.push_back(i);
		boost::posix_time::ptime t1=boost::posix_time::microsec_clock::local_time();
		parallel_sort_impl<std::vector<int>::iterator, std::less<int>, 42> s(0);
		s(v.begin(), v.end());
		boost::posix_time::ptime t2=boost::posix_time::microsec_clock::local_time();
		d=t2-t1;
	}

	boost::rand48 prng(42);
	while(true) {
		size_t size = (1 << (prng()%18)) + prng()%100;
		std::vector<int> v1;
		std::vector<int> v2;
		for (size_t i=0; i < size; ++i) {
			v1.push_back(prng());
			v2.push_back(v1.back());
		}
		std::cout << size << " ";

		boost::posix_time::ptime t1=boost::posix_time::microsec_clock::local_time();
		std::sort(v1.begin(), v1.end());
		boost::posix_time::ptime t2=boost::posix_time::microsec_clock::local_time();
		parallel_sort_impl<std::vector<int>::iterator, std::less<int>, 42> s(0);
		s(v2.begin(), v2.end());
		boost::posix_time::ptime t3=boost::posix_time::microsec_clock::local_time();
		if(v1 != v2) {std::cerr << "Failed" << std::endl; return;}
		std::cout << "std: " << (t2-t1) << " ours: " << t3-t2 << std::endl;
		if( std::max(d,(t2-t1)*3) < (t3-t2)  ) {std::cerr << "Too slow" << std::endl; return;}
	}
}



int main(int argc, char **argv) {
	tpie_initer _;
	progress = stdsort = true;
	--argc; ++argv;
	std::string test;
	while (argc) {
		std::string arg = argv[0];
		if (arg == "--no-progress") {
			progress = false;
		} else if (arg == "--no-stdsort") {
			stdsort = false;
		} else {
			test = arg;
			break;
		}
		--argc; ++argv;
	}
#define USAGE ("Usage: [--no-progress] [--no-stdsort] <basic1|basic2|medium|large|equal_elements|stress_test>")
	if (!argc) {
		std::cerr << USAGE << std::endl;
		return EXIT_FAILURE;
	}
	if (test == "basic1") {
		return basic1<2>(1024*1024) ? EXIT_SUCCESS : EXIT_FAILURE;
	} else if (test == "basic2") {
		return basic1<8>(8*8) ? EXIT_SUCCESS : EXIT_FAILURE;
	} else if (test == "medium") {
		return basic1<1024*1024>(1024*1024*24) ? EXIT_SUCCESS : EXIT_FAILURE;
	} else if (test == "large") {
		return basic1<1024*1024>(1024*1024*256) ? EXIT_SUCCESS : EXIT_FAILURE;
	} else if (test == "equal_elements") {
		exit(equal_elements()?EXIT_SUCCESS:EXIT_FAILURE);
	} else if (test == "stress_test") {
		stress_test();
		return EXIT_FAILURE;
	}

	std::cerr << USAGE << std::endl;
	return EXIT_FAILURE;
}
