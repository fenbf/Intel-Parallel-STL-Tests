
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
#include <pstl/memory>
#include <pstl/iterators.h>

#include "glm/vec4.hpp" // glm::vec4
#include "glm/geometric.hpp"
#include "glm/gtc/constants.hpp"

#include "benchmark/benchmark.h"


float GenRandomFloat(float lower, float upper)
{
	// usage of thread local random engines allows running the generator in concurrent mode
	thread_local static std::default_random_engine rd;
	std::uniform_real_distribution<float> dist(lower, upper);
	return dist(rd);
}

int GenRandomInt(int lower, int upper)
{
	// usage of thread local random engines allows running the generator in concurrent mode
	thread_local static std::default_random_engine rd;
	std::uniform_int_distribution<int> dist(lower, upper);
	return dist(rd);
}

template <typename Policy>
static void BM_Trigonometry(benchmark::State& state, Policy execution_policy)
{
	std::vector<double> vec(state.range(0), 0.5);
	std::generate(vec.begin(), vec.end(), []() { return GenRandomFloat(0.0f, 0.5f*glm::pi<float>()); });
	std::vector<double> out(vec);

	for (auto _ : state)
	{
		std::transform(execution_policy, vec.begin(), vec.end(), out.begin(),
			[](double v) {
			return std::sqrt(std::sin(v)*std::cos(v));
		}
		);
	}
}

BENCHMARK_CAPTURE(BM_Trigonometry, pstl_seq, pstl::execution::seq)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_Trigonometry, pstl_unseq, pstl::execution::unseq)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_Trigonometry, pstl_par, pstl::execution::par)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_Trigonometry, pstl_par_unseq, pstl::execution::par_unseq)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Unit(benchmark::kMicrosecond);

template <typename Policy>
static void BM_SortPoints(benchmark::State& state, Policy execution_policy)
{
	std::vector<glm::vec4> points(state.range(0), { 0.0f, 1.0f, 0.0f, 1.0f });
	std::generate(points.begin(), points.end(), []() {
		return glm::vec4(GenRandomFloat(-1.0f, 1.0f), GenRandomFloat(-1.0f, 1.0f), GenRandomFloat(-1.0f, 1.0f), 1.0f);
	});

	for (auto _ : state)
	{
		std::sort(execution_policy, points.begin(), points.end(),
			[](const glm::vec4& a, const glm::vec4& b) { return a.x < b.x; }
		);
	}
}

// sort only work with sequential and parallel execution, no vectorisation available
BENCHMARK_CAPTURE(BM_SortPoints, pstl_seq, pstl::execution::seq)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_SortPoints, pstl_par, pstl::execution::par)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);

template <typename Policy>
static void BM_DotProduct(benchmark::State& state, Policy execution_policy)
{
	std::vector<double> firstVec(state.range(0)), secondVec(state.range(0));

	std::generate(pstl::execution::par, firstVec.begin(), firstVec.end(), []() { return GenRandomFloat(-1.0f, 1.0f); });
	std::generate(pstl::execution::par, secondVec.begin(), secondVec.end(), []() { return GenRandomFloat(-1.0f, 1.0f); });

	for (auto _ : state)
	{
		double res = std::transform_reduce(execution_policy, firstVec.cbegin(), firstVec.cend(), secondVec.cbegin(), 0.0,
			std::plus<double>(), std::multiplies<double>());
		benchmark::DoNotOptimize(res);
	}
}

BENCHMARK_CAPTURE(BM_DotProduct, pstl_seq, pstl::execution::seq)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DotProduct, pstl_unseq, pstl::execution::unseq)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DotProduct, pstl_par, pstl::execution::par)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_DotProduct, pstl_par_unseq, pstl::execution::par_unseq)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);

template <typename Policy>
static void BM_CountingIter(benchmark::State& state, Policy execution_policy)
{
	const auto VecSize = state.range(0);
	std::vector<double> prices(VecSize);
	std::vector<unsigned int> quantities(VecSize);
	std::vector<double> discounts(VecSize);
	std::vector<double> profit(VecSize);

	for (auto _ : state)
	{
		std::for_each(execution_policy, pstl::counting_iterator<int64_t>(0), pstl::counting_iterator<int64_t>(VecSize),
			[&prices, &quantities, &discounts](int64_t i) {
			prices[i] = GenRandomFloat(0.5f, 100.0f);
			quantities[i] = GenRandomInt(1, 100);
			discounts[i] = GenRandomFloat(0.0f, 0.5f); // max 50%
		});

		std::transform(execution_policy, pstl::counting_iterator<int64_t>(0), pstl::counting_iterator<int64_t>(VecSize), profit.begin(),
			[&prices, &quantities, &discounts](int i) {
			return (prices[i] * (1.0f - discounts[i]))*quantities[i];
		});
	}
}

BENCHMARK_CAPTURE(BM_CountingIter, pstl_seq, pstl::execution::seq)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_CountingIter, pstl_unseq, pstl::execution::unseq)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_CountingIter, pstl_par, pstl::execution::par)->Arg(1000)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_CountingIter, pstl_par_unseq, pstl::execution::par_unseq)->RangeMultiplier(10)->Range(1000, 1000000)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();