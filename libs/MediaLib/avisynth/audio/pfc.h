#ifndef ___PFC_H___
#define ___PFC_H___


#include <windows.h>
#include <stdlib.h>

#define PFC_ALLOCA_LIMIT (4096)

#define INDEX_INVALID ((unsigned)(-1))

#include <malloc.h>

#include <tchar.h>
#include <stdio.h>

#include <assert.h>

#include <math.h>
#include <float.h>


#define NOVTABLE _declspec(novtable)

#define tabsize(x) (sizeof(x)/sizeof(*x))


template<class T>
class mem_ops
{
public:
	static void copy(T* dst, const T* src, unsigned count) { memcpy(dst, src, count * sizeof(T)); }
	static void move(T* dst, const T* src, unsigned count) { memmove(dst, src, count * sizeof(T)); }
	static void set(T* dst, int val, unsigned count) { memset(dst, val, count * sizeof(T)); }
	static void setval(T* dst, T val, unsigned count) { for (; count; count--) *(dst++) = val; }
	static T* alloc(unsigned count) { return reinterpret_cast<T*>(malloc(count * sizeof(T))); }
	static T* alloc_zeromem(unsigned count)
	{
		T* ptr = alloc(count);
		if (ptr) set(ptr, 0, count);
		return ptr;
	}
	static T* realloc(T* ptr, unsigned count)
	{
		return ptr ? reinterpret_cast<T*>(::realloc(reinterpret_cast<void*>(ptr), count * sizeof(T))) : alloc(count);
	}

	static void free(T* ptr) { ::free(reinterpret_cast<void*>(ptr)); }
	inline static T make_null_item()
	{
		char item[sizeof(T)];
		memset(&item, 0, sizeof(T));
		return *reinterpret_cast<T*>(&item);
	}
	inline static void swap(T& item1, T& item2) { T temp; temp = item1; item1 = item2; item2 = temp; }
};

class mem_block
{
public:
	enum mem_logic_t { ALLOC_DEFAULT, ALLOC_FAST, ALLOC_FAST_DONTGODOWN };
private:
	void* data;
	unsigned size, used;
	mem_logic_t mem_logic;
public:
	inline void set_mem_logic(mem_logic_t v) { mem_logic = v; }
	inline mem_logic_t get_mem_logic() const { return mem_logic; }

	inline mem_block() { data = 0; size = 0; used = 0; mem_logic = ALLOC_DEFAULT; }
	inline ~mem_block() { if (data) free(data); }

	inline unsigned get_size() const { return used; }

	inline const void* get_ptr() const { return data; }
	inline void* get_ptr() { return data; }

	void* set_size(UINT new_used)
	{
		if (new_used == 0)
		{
			if (mem_logic != ALLOC_FAST_DONTGODOWN)
			{
				if (data != 0) { free(data); data = 0; }
				size = 0;
			}
		}
		else
		{
			UINT new_size;
			if (mem_logic == ALLOC_FAST || mem_logic == ALLOC_FAST_DONTGODOWN)
			{
				new_size = size;
				if (new_size < 1) new_size = 1;
				while (new_size < new_used) new_size <<= 1;
				if (mem_logic != ALLOC_FAST_DONTGODOWN) while (new_size >> 1 > new_used) new_size >>= 1;
			}
			else
			{
				new_size = new_used;
			}

			if (new_size != size)
			{
				if (data == 0)
				{
					data = malloc(new_size);
				}
				else
				{
					void* new_data;
#if defined(_DEBUG) && 0
					new_data = malloc(new_size);
					if (new_data) memcpy(new_data, data, new_size > size ? size : new_size);
					if (size >= 4) *(DWORD*)data = 0xDEADBEEF;
					free(data);
					data = new_data;
#else
					new_data = realloc(data, new_size);
					if (new_data == 0) free(data);
					data = new_data;
#endif
				}
				size = new_size;
			}
		}
		used = new_used;
		return data;
	}

	inline void* check_size(unsigned new_size)
	{
		if (used < new_size) return set_size(new_size);
		else return get_ptr();
	}

	inline operator const void* () const { return get_ptr(); }
	inline operator void* () { return get_ptr(); }

	void* copy(const void* ptr, unsigned bytes, unsigned start = 0);

	inline void* append(const void* ptr, unsigned bytes) { return copy(ptr, bytes, used); }

	inline void zeromemory() { memset(get_ptr(), 0, used); }

	inline void force_reset() { if (data) free(data); data = 0; size = 0; used = 0; }

	static void g_apply_order(void* data, unsigned size, const int* order, unsigned num);
};

template<class T>
class mem_block_t //: public mem_block	
{
	mem_block theBlock;//msvc7 sucks
public:
	mem_block_t() {}
	mem_block_t(unsigned s) { theBlock.set_size(s * sizeof(T)); }

	inline void set_mem_logic(mem_block::mem_logic_t v) { theBlock.set_mem_logic(v); }
	inline mem_block::mem_logic_t get_mem_logic() const { return theBlock.get_mem_logic(); }

	inline unsigned get_size() const { return theBlock.get_size() / sizeof(T); }

	inline const T* get_ptr() const { return static_cast<const T*>(theBlock.get_ptr()); }
	inline T* get_ptr() { return static_cast<T*>(theBlock.get_ptr()); }

	inline T* set_size(unsigned new_size) { return static_cast<T*>(theBlock.set_size(new_size * sizeof(T))); }

	inline T* check_size(unsigned new_size) { return static_cast<T*>(theBlock.check_size(new_size * sizeof(T))); }

	inline operator const T* () const { return get_ptr(); }
	inline operator T* () { return get_ptr(); }

	inline T* copy(const T* ptr, unsigned size, unsigned start = 0) { return static_cast<T*>(theBlock.copy(static_cast<const void*>(ptr), size * sizeof(T), start * sizeof(T))); }
	inline T* append(const T* ptr, unsigned size) { return static_cast<T*>(theBlock.append(static_cast<const void*>(ptr), size * sizeof(T))); }
	inline void append(T item) { theBlock.append(static_cast<const void*>(&item), sizeof(item)); }

	inline void swap(unsigned idx1, unsigned idx2)
	{
		T* ptr = get_ptr();
		mem_ops<T>::swap(ptr[idx1], ptr[idx2]);
	}


	unsigned write_circular(unsigned offset, const T* src, unsigned count)
	{//returns new offset
		unsigned total = get_size();
		if (offset < 0)
		{
			offset = total - ((-(int)offset) % total);
		}
		else offset %= total;

		if (count > total)
		{
			src += count - total;
			count = total;
		}

		while (count > 0)
		{
			unsigned delta = count;
			if (delta > total - offset) delta = total - offset;
			unsigned n;
			for (n = 0; n < delta; n++)
				get_ptr()[n + offset] = *(src++);
			count -= delta;
			offset = (offset + delta) % total;
		}
		return offset;
	}

	unsigned read_circular(unsigned offset, T* dst, unsigned count)
	{
		unsigned total = get_size();
		if (offset < 0)
		{
			offset = total - ((-(int)offset) % total);
		}
		else offset %= total;

		while (count > 0)
		{
			unsigned delta = count;
			if (delta > total - offset) delta = total - offset;
			unsigned n;
			for (n = 0; n < delta; n++)
				*(dst++) = get_ptr()[n + offset];
			count -= delta;
			offset = (offset + delta) % total;
		}
		return offset;
	}

	inline void zeromemory() { theBlock.zeromemory(); }

	void fill(T val)
	{
		unsigned n = get_size();
		T* dst = get_ptr();
		for (; n; n--) *(dst++) = val;
	}

	inline void force_reset() { theBlock.force_reset(); }

	void apply_order(const int* order, unsigned num)
	{
		assert(num <= get_size());
		mem_block::g_apply_order(get_ptr(), sizeof(T), order, num);
	}

};




template<class T>
class mem_block_aligned_t
{
	mem_block data;
	T* ptr_aligned;
public:
	mem_block_aligned_t() { ptr_aligned = 0; }
	mem_block_aligned_t(unsigned size) { ptr_aligned = 0; set_size(size); }

	unsigned get_size() const { return data.get_size() / sizeof(T); }

	inline void set_mem_logic(mem_block::mem_logic_t v) { data.set_mem_logic(v); }
	inline mem_block::mem_logic_t get_mem_logic() const { return data.get_mem_logic(); }

	T* set_size(unsigned size)
	{
		unsigned size_old = get_size();
		int delta_old = (unsigned)ptr_aligned - (unsigned)data.get_ptr();
		unsigned ptr = (unsigned)data.set_size((size + 1) * sizeof(T) - 1), old_ptr = ptr;
		ptr += sizeof(T) - 1;
		ptr -= ptr % sizeof(T);
		int delta_new = ptr - old_ptr;
		if (delta_new != delta_old)
		{
			unsigned to_move = size_old > size ? size : size_old;
			memmove((char*)ptr, (char*)ptr - (delta_new - delta_old), to_move * sizeof(T));
		}
		ptr_aligned = (T*)ptr;
		return ptr_aligned;
	}

	T* check_size(unsigned size)
	{
		return size > get_size() ? set_size(size) : get_ptr();
	}

	void fill(T val)
	{
		unsigned n = get_size();
		T* dst = get_ptr();
		for (; n; n--) *(dst++) = val;
	}

	inline void zeromemory() { data.zeromemory(); }
	inline const T* get_ptr() const { return ptr_aligned; }
	inline T* get_ptr() { return ptr_aligned; }
	inline operator const T* () const { return get_ptr(); }
	inline operator T* () { return get_ptr(); }
};


template<class T>
class mem_block_list_t
{
private:
	mem_block_t<T> buffer;
public:
	mem_block_list_t() { buffer.set_mem_logic(mem_block::ALLOC_FAST); }
	inline void set_mem_logic(mem_block::mem_logic_t v) { buffer.set_mem_logic(v); }

	unsigned add_items_repeat(T item, unsigned count)//returns first index
	{
		int idx = buffer.get_size();
		buffer.set_size(idx + count);
		unsigned n;
		for (n = 0; n < count; n++)
			buffer[idx + n] = item;
		return idx;
	}


	void insert_item(T item, unsigned idx)
	{
		unsigned max = buffer.get_size() + 1;
		buffer.set_size(max);
		unsigned n;
		for (n = max - 1; n > idx; n--)
			buffer[n] = buffer[n - 1];
		buffer[idx] = item;
	}

	void insert_item_ref(const T& item, unsigned idx)
	{
		unsigned max = buffer.get_size() + 1;
		buffer.set_size(max);
		unsigned n;
		for (n = max - 1; n > idx; n--)
			buffer[n] = buffer[n - 1];
		buffer[idx] = item;
	}

	T remove_by_idx(unsigned idx)
	{
		T ret = buffer[idx];
		int n;
		int max = buffer.get_size();
		for (n = idx + 1; n < max; n++)
		{
			buffer[n - 1] = buffer[n];
		}
		buffer.set_size(max - 1);
		return ret;
	}


	inline T get_item(unsigned n) const
	{
		assert(n >= 0);
		assert(n < get_count());
		return buffer[n];
	}
	inline unsigned get_count() const { return buffer.get_size(); }
	inline T operator[](unsigned n) const
	{
		assert(n >= 0);
		assert(n < get_count());
		return buffer[n];
	}
	inline const T* get_ptr() const { return buffer.get_ptr(); }
	inline T& operator[](unsigned n) { return buffer[n]; }


	void insert_items_fromptr(const T* source, int num, int idx)
	{
		int count = get_count();
		if (idx < 0 || idx >= count) add_items_fromptr(source, num);
		else
		{
			buffer.set_size(count + num);
			int n;
			for (n = count - 1; n >= idx; n--)
			{
				buffer[n + num] = buffer[n];
			}
			for (n = 0; n < num; n++)
			{
				buffer[n + idx] = source[n];
			}
			count += num;
		}
	}

	inline void add_items(const mem_block_list_t<T>& source)
	{
		add_items_fromptr(source.buffer, source.get_count());
	}

	void add_items_fromptr(const T* source, unsigned num)
	{
		unsigned count = get_count();
		buffer.set_size(count + num);
		unsigned n;
		for (n = 0; n < num; n++)
		{
			buffer[count++] = source[n];
		}
	}

	inline T replace_item(unsigned idx, T item)
	{
		assert(idx >= 0);
		assert(idx < get_count());
		T ret = buffer[idx];
		buffer[idx] = item;
		return ret;
	}

	void sort(int(__cdecl* compare)(const T* elem1, const T* elem2))
	{
		qsort((void*)get_ptr(), get_count(), sizeof(T), (int(__cdecl*)(const void*, const void*))compare);
	}

	bool bsearch(int(__cdecl* compare)(T elem1, T elem2), T item, int* index) const
	{
		int max = get_count();
		int min = 0;
		int ptr;
		while (min < max)
		{
			ptr = min + (max - min) / 2;
			T& test = buffer[ptr];
			int result = compare(test, item);
			if (result < 0) min = ptr + 1;
			else if (result > 0) max = ptr;
			else
			{
				if (index) *index = ptr;
				return 1;
			}
		}
		if (index) *index = min;
		return 0;
	}

	bool bsearch_ref(int(__cdecl* compare)(T const& elem1, T const& elem2), T const& item, int* index) const
	{
		int max = get_count();
		int min = 0;
		int ptr;
		while (min < max)
		{
			ptr = min + (max - min) / 2;
			T const& test = buffer[ptr];
			int result = compare(test, item);
			if (result < 0) min = ptr + 1;
			else if (result > 0) max = ptr;
			else
			{
				if (index) *index = ptr;
				return 1;
			}
		}
		if (index) *index = min;
		return 0;
	}


	bool bsearch_range(int(__cdecl* compare)(T elem1, T elem2), T item, int* index, int* count) const
	{
		int max = get_count();
		int min = 0;
		int ptr;
		while (min < max)
		{
			ptr = min + (max - min) / 2;
			T& test = buffer[ptr];
			int result = compare(test, item);
			if (result < 0) min = ptr + 1;
			else if (result > 0) max = ptr;
			else
			{
				int num = 1;
				while (ptr > 0 && !compare(buffer[ptr - 1], item)) { ptr--; num++; }
				while (ptr + num < get_count() && !compare(buffer[ptr + num], item)) num++;
				if (index) *index = ptr;
				if (count) *count = num;
				return 1;
			}
		}
		if (index) *index = min;
		return 0;
	}

	bool bsearch_param(int(__cdecl* compare)(T elem1, const void* param), const void* param, int* index) const
	{
		int max = get_count();
		int min = 0;
		int ptr;
		while (min < max)
		{
			ptr = min + (max - min) / 2;
			T test = buffer[ptr];
			int result = compare(test, param);
			if (result < 0) min = ptr + 1;
			else if (result > 0) max = ptr;
			else
			{
				if (index) *index = ptr;
				return 1;
			}
		}
		if (index) *index = min;
		return 0;
	}




	inline void apply_order(const int* order, unsigned count)
	{
		assert(count == get_count());
		buffer.apply_order(order, count);
	}

	unsigned add_item_ref(const T& item)
	{
		unsigned idx = get_count();
		insert_item_ref(item, idx);
		return idx;
	}

	unsigned add_item(T item)//returns index
	{
		unsigned idx = get_count();
		insert_item(item, idx);
		return idx;
	}


	int find_item(T item) const//returns index of first occurance, -1 if not found
	{
		unsigned n, max = get_count();
		for (n = 0; n < max; n++)
			if (get_item(n) == item) return n;
		return -1;
	}

	inline bool have_item(T item) const { return find_item(item) >= 0; }

	inline void swap(unsigned idx1, unsigned idx2)
	{
		replace_item(idx1, replace_item(idx2, get_item(idx1)));
	}

};

#define mem_block_list mem_block_list_t //for compatibility


#endif //___PFC_H___