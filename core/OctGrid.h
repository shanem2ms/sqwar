#pragma once

#include <gmtl/gmtl.h>
#include <gmtl/Vec.h>

using namespace gmtl;
typedef unsigned int uint32;
typedef unsigned char uchar;

template <typename T> class GridEmptyVal
{

};

template <int D, int C, typename T> class Grid
{
public:
	typedef uint32 size_idx;
private:

	template <int A, int B>
	struct PowC
	{
		static const int Val = A * PowC<A, B - 1>::Val;
	};
	template <int A>
	struct PowC<A, 0>
	{
		static const int Val = 1;
	};

	static constexpr bool IsPower2C(int v) {
		return v && ((v & (v - 1)) == 0);
	}

	static constexpr size_idx Log2C(size_t n)
	{
		static_assert(IsPower2C(C));
		return ((n < 2) ? 0 : 1 + Log2C(n / 2));
	}

public:
	inline static const T emptyVal = GridEmptyVal<T>::Val;
	static const size_idx GRIDSIZE = PowC<C, D>::Val;
	static const size_idx CS = Log2C(C);
	static const size_idx SizeBytes = GRIDSIZE * sizeof(T);
private:
	T* data;

	/*
	struct Weights
	{
		T vals[D * 2];

		size_idx locs[D * 2];
		size_idx idx;
		bool isVal;

		Weights()
		{
			isVal = false;
			memset(locs, 0, sizeof(locs));
			memset(vals, emptyVal, sizeof(vals));
		}
	};
	*/
public:

	inline void EnsureData() const
	{
		if (data == nullptr)
		{
			const_cast<Grid*>(this)->data =
				new T[GRIDSIZE];
			const_cast<Grid*>(this)->
				SetAllValues(emptyVal);
		}
	}
	Grid() :
		data(nullptr)
	{
	}

	Grid(const T& initVal)
	{
		data = new T[GRIDSIZE];
		SetAllValues(initVal);
	}

	~Grid()
	{
		Clear();
	}

	inline static bool HasVal(T* ptr)
	{
		return memcmp(ptr, &emptyVal, sizeof(emptyVal)) != 0;
	}

	inline static void SetEmpty(T* ptr)
	{
		static_assert(sizeof(emptyVal) == sizeof(T));
		memcpy(ptr, &emptyVal, sizeof(T));
	}

	inline static bool ValIsEmpty(const T* v)
	{
		return memcmp(v, &emptyVal, sizeof(T)) == 0;
	}

	inline void SetAllValues(const T& val)
	{
		EnsureData();
		std::fill(data, data + GRIDSIZE, val);
	}


	inline bool IsEmpty()
	{
		if (data == nullptr)
			return true;

		for (size_t idx = 0; idx < GRIDSIZE; ++idx)
		{
			if (!ValIsEmpty(&data[idx]))
				return false;
		}

		return true;
	}

	void Clear()
	{
		if (data != nullptr)
			delete[]data;
		data = nullptr;
	}

	T& operator[](int X) { EnsureData();  return data[X]; }

	const T& operator[](int X) const { EnsureData(); return data[X]; }

	static size_idx IDX(size_idx x, size_idx y, size_idx z) {
		static_assert(D == 3);
		return ((z * C * C) + (y * C) + x);
	}

	static void XYZ(size_idx idx, size_idx& x, size_idx& y, size_idx& z) {
		static_assert(D == 3);
		z = (idx >> (CS * 2)) & (C - 1);
		y = (idx >> CS) & (C - 1);
		x = idx & (C - 1);
	}

	inline const T& Val(size_idx X, size_idx Y, size_idx Z) const {
		EnsureData();
		return data[IDX(X, Y, Z)];
	}

	inline T& Val(size_idx X, size_idx Y, size_idx Z) {
		EnsureData();
		return data[IDX(X, Y, Z)];
	}

	inline void Val(size_idx X, size_idx Y, size_idx Z, const T& val)
	{
		EnsureData();
		data[IDX(X, Y, Z)] = val;
	}

	template <int L> void LerpGrid();
	template <int C2> void CopyGridAtLoc(size_idx x, size_idx y, size_idx z,
		const Grid<D, C2, T>& outGrid);
	template<int S1, int S2, int C2> void CopyIntoRow(T* dest, const T* src);

	template<int S> void DownsampleRow(T* datagrid);
	template <int C2> void DownSample(Grid<D, C2, T>& outGrid);
	template <int S, int L> void LerpRow(T* beginPtr, float* weightBgnPtr);

	template <typename ST> void MapTo(Grid<D, C, ST>& outGrid, T& minRg, T& maxRg);
};

template <> class GridEmptyVal<float>
{
public:
	static const uint32 emptyValHx;
	static const float Val;
};

template <> class GridEmptyVal<uint32>
{
public:
	static const uint32 emptyValHx;
	static const uint32 Val;
};

template <> class GridEmptyVal<uchar>
{
public:
	static const uchar Val;
};


template <> class GridEmptyVal<Vec3f>
{
public:
	static const uint32 emptyValHx;
	static const Vec3f Val;
};
