
#include <algorithm>
#include <chrono>
#include <execution>
#include <iostream>
#include <vector>
#include <random>
#include <set>

#include <pstl/algorithm>
#include <pstl/numeric>
#include <pstl/execution>

#include "glm/vec4.hpp" // glm::vec4
#include "glm/geometric.hpp"
#include "glm/gtc/constants.hpp"

constexpr int RUN_TIMES = 5;

template <typename TFunc> void RunAndMeasure(const char* title, TFunc func)
{
	std::set<double> times;
	std::vector results(RUN_TIMES, func()); // invoke it for the first time...
	for (int i = 0; i < RUN_TIMES; ++i)
	{
		const auto start = std::chrono::steady_clock::now();
		const auto ret = func();
		const auto end = std::chrono::steady_clock::now();
		results[i] = ret;
		times.insert(std::chrono::duration<double, std::milli>(end - start).count());
	}

	std::cout << title << ":\t " << *times.begin() << "ms (max was " << *times.rbegin() << ") " << results[0] << '\n';
}

float GenRandomFloat(float lower, float upper)
{
	// usage of thread local random engines allows running the generator in concurrent mode
	thread_local static std::default_random_engine rd;
	std::uniform_real_distribution<float> dist(lower, upper);
	return dist(rd);
}

double random_number_generator() {
	// usage of thread local random engines allows running the generator in concurrent mode
	thread_local static std::default_random_engine rd;
	std::uniform_real_distribution<double> dist(0, 1);
	return dist(rd);
}

void TestTrig(const size_t vecSize)
{
	std::vector<double> vec(vecSize, 0.5);
	std::generate(vec.begin(), vec.end(), []() { return GenRandomFloat(0.0f, 0.5f*glm::pi<float>()); });
	std::vector out(vec);

	std::cout << "sqrt(sin*cos):\n";

	RunAndMeasure("pstl::execution::seq", [&vec, &out] {
		std::transform(pstl::execution::seq, vec.begin(), vec.end(), out.begin(),
			[](double v) {
			return std::sqrt(std::sin(v)*std::cos(v));
		}
		);
		return out[0];
	});

	RunAndMeasure("pstl::execution::par", [&vec, &out] {
		std::transform(pstl::execution::par, vec.begin(), vec.end(), out.begin(),
			[](double v) {
			return std::sqrt(std::sin(v)*std::cos(v));
		}
		);
		return out[0];
	});

	RunAndMeasure("std::transform seq", [&vec, &out] {
		std::transform(std::execution::seq, vec.begin(), vec.end(), out.begin(),
			[](double v) {
			return std::sqrt(std::sin(v)*std::cos(v));
		}
		);
		return out[0];
	});

	RunAndMeasure("std::transform par", [&vec, &out] {
		std::transform(std::execution::par, vec.begin(), vec.end(), out.begin(),
			[](double v) {
			return std::sqrt(std::sin(v)*std::cos(v));
		}
		);
		return out[0];
	});
}

void TestDotProduct(const size_t vecSize)
{
	std::cout << "dot product:\n";

	std::vector<double> v1(vecSize), v2(vecSize);

	//initialize vectors with random numbers
	std::generate(pstl::execution::par, v1.begin(), v1.end(), random_number_generator);
	std::generate(pstl::execution::par, v2.begin(), v2.end(), random_number_generator);

	RunAndMeasure("pstl::execution::seq", [&v1, &v2] {
		double res = std::transform_reduce(pstl::execution::seq, v1.cbegin(), v1.cend(), v2.cbegin(), .0,
			std::plus<double>(), std::multiplies<double>());
		return res;
	});

	RunAndMeasure("pstl::execution::par", [&v1, &v2] {
		double res = std::transform_reduce(pstl::execution::par, v1.cbegin(), v1.cend(), v2.cbegin(), .0,
			std::plus<double>(), std::multiplies<double>());
		return res;
	});

	RunAndMeasure("std::execution::seq", [&v1, &v2] {
		double res = std::transform_reduce(std::execution::seq, v1.cbegin(), v1.cend(), v2.cbegin(), .0,
			std::plus<double>(), std::multiplies<double>());
		return res;
	});

	RunAndMeasure("std::execution::par", [&v1, &v2] {
		double res = std::transform_reduce(std::execution::par, v1.cbegin(), v1.cend(), v2.cbegin(), .0,
			std::plus<double>(), std::multiplies<double>());
		return res;
	});
}

int main(int argc, char* argv[])
{
	const size_t vecSize = argc > 1 ? atoi(argv[1]) : 6000000;
	std::cout << vecSize << '\n';

	int step = argc > 2 ? atoi(argv[2]) : 0;

	if (step == 0 || step == 2)
		TestTrig(vecSize);

	if (step == 0 || step == 3)
		TestDotProduct(vecSize);

	return 0;
}