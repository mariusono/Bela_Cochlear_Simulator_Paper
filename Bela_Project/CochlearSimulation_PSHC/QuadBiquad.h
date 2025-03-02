#pragma once
#include <array>
#include <arm_neon.h>
#include <new>
#include <stdlib.h>
#include <stdio.h>

/**
 * A class which processes four biquad filters in parallel in an optimised way.
 *
 * These filters use `float` data types internally and the process() routine
 * uses NEON intrinsics.
 */
class QuadBiquad
{
public:
	static unsigned int constexpr kNumFilters = 4;

	std::array<BiquadCoeffT<float>, kNumFilters> filters;

	/**
	 * Construct the object.
	 *
	 * May fail with `std::bad_alloc` if the provided memory is not
	 * properly aligned. This may occur when using a version of C++ prior
	 * to c++17 and doing heap allocation, e.g.: in an STL container (e.g.:
	 * `std::vector`). Use `new` to guarantee properly aligned memory.
	 */
	QuadBiquad()
	{
		if(size_t(this) & size_t(alignof(QuadBiquad) - 1))
		{
			fprintf(stderr, "QuadBiquad object is improperly aligned. Avoid heap allocation, use operator new or use -std=c++17\n");
			std::bad_alloc e;
			throw(e);
		}
	}


	void update()
	{
	#define QUADBIQUAD_UPDATE(arg,sign) \
	{ \
		float32_t data[4] = {sign float32_t(filters[0].arg), sign float32_t(filters[1].arg), sign float32_t(filters[2].arg), sign float32_t(filters[3].arg)}; \
		arg = vld1q_f32(data); \
	}
		QUADBIQUAD_UPDATE(a0,+);
		QUADBIQUAD_UPDATE(a1,+);
		QUADBIQUAD_UPDATE(a2,+);
		QUADBIQUAD_UPDATE(b1,-);
		QUADBIQUAD_UPDATE(b2,-);
	}
			
			
						/**
	 * Allocate the object's memory so that it respects the desired alignment
	 */
	void* operator new(size_t sz) {
		auto ptr = aligned_alloc(alignof(QuadBiquad), sz);
		if(!ptr)
		{
			std::bad_alloc e;
			throw(e);
		}
		return ptr;
	}
	
	int setup(const )
	{
		int ret = 0;
		for(auto & b : filters)
			ret |= b.setup(settings);
		update();
		z1 = vmovq_n_f32(0);
		z2 = vmovq_n_f32(0);
		return ret;
	}
	

	/**
	 * Initialise the four filters and the QuadBiquad internals.
	 *
	 * Alternatively, call setup() on each of the elements of #filters and
	 * then call update().
	 */
	int setup(const BiquadCoeff::Settings& settings);

	/**
	 * Call this after manipulating the filter parameters of one or more of
	 * the individual elements of #filters and before calling process().
	 */
	void update();

	/**
	 * Process input data through the filter.
	 *
	 * @param data input / output. Each element of the vector corresponds
	 * to the corresponding filtering channel in #filters.
	 */
	void process(float data[kNumFilters])
	{
		// See here https://developer.arm.com/architectures/instruction-sets/simd-isas/neon/intrinsics
		// for more on ARM intrinsics
		float32x4_t in = vld1q_f32(data);
		// We need to do the following: (see Biquad::process)
		// out = in * a0 + z1; (A)
		// z1 = in * a1 + z2 - b1 * out; split into two(B, C):
		// z2 = in * a2 - b2 * out; split into two(D, E):
		// store the output (F)
		// These are interleaved below to avoid NEON to stall while
		// waiting for results.

		// A: out = in * a0 + z1;
		float32x4_t out = vmlaq_f32(z1, in, a0);
		// B: z1 = in * a1 + z2;
		z1 = vmlaq_f32(z2, in, a1);
		// D: z2 = in * a2;
		z2 = vmulq_f32(in, a2);
		// F: store the output
		vst1q_f32(data, out);
		// C: z1 = z1 - b1 * out; ***
		z1 = vmlaq_f32(z1, b1, out);
		// E: z2 = z2 - b2 * out; ***
		z2 = vmlaq_f32(z2, b2, out);
		// ***: note that we inverted the sign of the b1 and b2 coefficients
		// in update() and we use vmlaq instead of vmlsq here. This is
		// because vmlaq seems to be slightly faster than vmlsq.
	}
private:
	float32x4_t z1;
	float32x4_t z2;
	float32x4_t a0;
	float32x4_t a1;
	float32x4_t a2;
	float32x4_t b1;
	float32x4_t b2;
};
extern template class BiquadCoeffT<float>;