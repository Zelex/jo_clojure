#pragma once

template<typename T>
struct jo_alloc_t;

struct jo_alloc_base_t {
	virtual void free(void *n) = 0;
};

template<typename T>
struct jo_alloc_t_type {
    struct header_t {
        jo_alloc_base_t *owner;
        unsigned idx;
        unsigned next;
        std::atomic<int> ref_count = 0;
        unsigned canary;
    } h;
    char data[sizeof(T)];

    jo_alloc_t_type() {}
    jo_alloc_t_type(const jo_alloc_t_type &other) {} 
    jo_alloc_t_type(const jo_alloc_t_type &&other) {} 

    void add_ref() {
        assert(h.canary == 0xDEADBEEF);
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
        ++h.ref_count;
#else
        h.ref_count.fetch_add(1, std::memory_order_relaxed);
#endif
    }

    void release() {
        h.owner->free(data);
    }
};

template<typename T> 
struct jo_shared_ptr_t;

// allocate specific types multi threaded very quickly.
template<typename T>
struct jo_alloc_t : jo_alloc_base_t {
    typedef jo_alloc_t_type<T> T_t;

    enum {
        MAX_SECTORS = 256
    };

    const int NUM_SECTORS = jo_min(std::thread::hardware_concurrency(), MAX_SECTORS);
	jo_pinned_vector<T_t> vec;
    char pad[64];
    // Make sure each atomic is on its own cache-line
    struct {
        std::atomic<unsigned long long> head;
        char pad[64 - sizeof(std::atomic<unsigned long long>)];
    } free_list[MAX_SECTORS];

    jo_alloc_t() {
        for (int i = 0; i < NUM_SECTORS; i++) {
            free_list[i].head.store(0, std::memory_order_relaxed);
        }
    }

    jo_alloc_t(const jo_alloc_t &other) {
        for (int i = 0; i < NUM_SECTORS; i++) {
            free_list[i].head.store(0, std::memory_order_relaxed);
        }
    }

    T *alloc() {
        unsigned sector = thread_id & (NUM_SECTORS-1);
        for(int i = 0; i < 1; ++i) {
            unsigned long long cnt_idx = free_list[sector].head.load();
            unsigned long long n_idx = cnt_idx & 0xffffffff;
            while (n_idx > 0) {
                T_t *n = &vec[n_idx-1];
                unsigned long long cnt = (cnt_idx + 0x100000000ull) & (-1ll << 32);
                unsigned long long cnt_idx2 = n->h.next | cnt;
                if(free_list[sector].head.compare_exchange_weak(cnt_idx, cnt_idx2, std::memory_order_relaxed)) {
                    assert(n->h.canary == 0);
                    n->h.canary = 0xDEADBEEF;
                    n->h.next = 0;
                    n->h.ref_count.store(0);
                    n->h.owner = this;
                    //assert(!memcmp(n->data, "\xcd\xcd\xcd\xcd", 4)); // DEBUG
                    return (T*)n->data;
                }
                n_idx = cnt_idx & 0xffffffff;
            }
            sector = jo_pcg32(&jo_rnd_state) & (NUM_SECTORS-1);
        }
		size_t idx = vec.push_back(std::move(T_t()));
		T_t *n = &vec[idx];
		n->h.canary = 0xDEADBEEF;
        n->h.idx = idx+1;
		n->h.next = 0;
		n->h.ref_count.store(0);
        n->h.owner = this;
		return (T*)n->data;
	}

	template<typename...A>
	T *emplace(A... args) {
		return new(alloc()) T(args...);
	}

	void free(void *n) {
		T_t *t = (T_t*)((char*)n - sizeof(t->h));
        assert(t->h.canary == 0xDEADBEEF);
		if(t->h.ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			assert(t->h.canary == 0xDEADBEEF);
			t->h.canary = 0; // make sure it's not double free'd
            t->h.owner = 0;
            // call dtor if not pod type
			if(!std::is_pod<T>::value) {
				((T*)t->data)->~T();
			}
            //memset(t->data, 0xCD, sizeof(T)); // DEBUG
            unsigned idx = t->h.idx;
            unsigned sector = thread_id & (NUM_SECTORS-1);
            unsigned long long old_h = free_list[sector].head.load(), new_h;
            do {
                t->h.next = old_h & 0xffffffff;
                new_h = (old_h & 0xffffffff00000000) | idx;
            } while(!free_list[sector].head.compare_exchange_weak(old_h, new_h, std::memory_order_relaxed));
		}
	}
};

template<typename T> 
struct jo_shared_ptr_t {
    typedef jo_alloc_t_type<T> T_t;
    T* ptr;
    
    jo_shared_ptr_t() : ptr(nullptr) {}
    jo_shared_ptr_t(T* Ptr) : ptr(Ptr) {
        if(ptr) ((T_t*)((char*)ptr - sizeof(typename T_t::header_t)))->add_ref();
    }
    jo_shared_ptr_t(const jo_shared_ptr_t& other) : ptr(other.ptr) {
        if(ptr) ((T_t*)((char*)ptr - sizeof(typename T_t::header_t)))->add_ref();
    }
    jo_shared_ptr_t(jo_shared_ptr_t&& other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    jo_shared_ptr_t& operator=(const jo_shared_ptr_t& other) {
        if (this != &other) {
            if(other.ptr) ((T_t*)((char*)other.ptr - sizeof(typename T_t::header_t)))->add_ref();
            if(ptr) ((T_t*)((char*)ptr - sizeof(typename T_t::header_t)))->release();
            ptr = other.ptr;
        }
        return *this;
    }
    
    jo_shared_ptr_t& operator=(jo_shared_ptr_t&& other) {
        if (this != &other) {
            if(ptr) ((T_t*)((char*)ptr - sizeof(typename T_t::header_t)))->release();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    
    ~jo_shared_ptr_t() {
        if(ptr) ((T_t*)((char*)ptr - sizeof(typename T_t::header_t)))->release();
        ptr = nullptr;
    }

    T& operator*() { return *ptr; }
    T* operator->() { return ptr; }
    const T& operator*() const { return *ptr; }
    const T* operator->() const { return ptr; }

    bool operator==(const jo_shared_ptr_t& other) const { return ptr == other.ptr; }
    bool operator!=(const jo_shared_ptr_t& other) const { return ptr != other.ptr; }

    bool operator!() const { return ptr == nullptr; }
    operator bool() const { return ptr != nullptr; }

    //operator T&() { return *ptr; }
    //operator T&() const { return *ptr; }

    template<typename U> jo_shared_ptr_t<U> &cast() { 
        static_assert(std::is_base_of<T, U>::value || std::is_base_of<U, T>::value, "type parameter of this class must derive from T or U");
        return (jo_shared_ptr_t<U>&)(*this); 
    }
    template<typename U> const jo_shared_ptr_t<U> &cast() const { 
        static_assert(std::is_base_of<T, U>::value || std::is_base_of<U, T>::value, "type parameter of this class must derive from T or U");
        return (const jo_shared_ptr_t<U>&)(*this); 
    }
};

#ifdef USE_64BIT_NODES
typedef long long node_idx_unsafe_t;
#else
typedef int node_idx_unsafe_t;
#endif

static inline void node_add_ref(node_idx_unsafe_t idx);
static inline void node_release(node_idx_unsafe_t idx);

struct node_idx_t {
	node_idx_unsafe_t idx;
	node_idx_t() : idx() {}
	node_idx_t(node_idx_unsafe_t _idx) : idx(_idx) { 
		node_add_ref(idx); 
	}
	~node_idx_t() { 
		node_release(idx); 
	}
	node_idx_t(const node_idx_t& other) : idx(other.idx) { 
		node_add_ref(idx); 
	}
	node_idx_t(node_idx_t&& other) : idx(other.idx) { 
		other.idx = 0; 
	}
	node_idx_t& operator=(node_idx_unsafe_t _idx) {  
		if(idx != _idx) {
			node_add_ref(_idx);
			node_release(idx);
			idx = _idx;
		}
		return *this; 
	}
	node_idx_t& operator=(const node_idx_t& other) {  
		if(idx != other.idx) {
			node_add_ref(other.idx);
			node_release(idx);
			idx = other.idx;
		}
		return *this; 
	}
	node_idx_t& operator=(node_idx_t&& other) {  
		if(idx != other.idx) {
			node_release(idx);
			idx = other.idx;
			other.idx = 0;
		}
		return *this; 
	}

	operator node_idx_unsafe_t() const { return idx; }
	bool operator==(const node_idx_t& other) const { return idx == other.idx; }
	bool operator!=(const node_idx_t& other) const { return idx != other.idx; }
	bool operator==(node_idx_unsafe_t _idx) const { return idx == _idx; }
	bool operator!=(node_idx_unsafe_t _idx) const { return idx != _idx; }
#ifdef USE_64BIT_NODES
	bool operator==(int _idx) const { return idx == _idx; }
	bool operator!=(int _idx) const { return idx != _idx; }
#endif
};

struct atomic_node_idx_t {
	std::atomic<node_idx_unsafe_t> idx;
	atomic_node_idx_t() : idx() {}
	~atomic_node_idx_t() { 
		node_release(idx.load()); 
	}
	atomic_node_idx_t(const atomic_node_idx_t& other) { 
		idx.store(other.idx.load());
		node_add_ref(idx.load()); 
	}
	atomic_node_idx_t(const node_idx_t& other) { 
		idx.store(other.idx);
		node_add_ref(idx); 
	}
	atomic_node_idx_t(atomic_node_idx_t&& other) { 
		idx.store(other.idx.load());
		other.idx.store(0); 
	}
	atomic_node_idx_t& operator=(const node_idx_t& other) {  
		if(idx != other.idx) {
			node_add_ref(other.idx);
			node_release(idx);
			idx.store(other.idx);
		}
		return *this; 
	}
	atomic_node_idx_t& operator=(const atomic_node_idx_t& other) {  
		if(idx != other.idx) {
			node_add_ref(other.idx);
			node_release(idx);
			idx.store(other.idx.load());
		}
		return *this; 
	}
	atomic_node_idx_t& operator=(atomic_node_idx_t&& other) {  
		if(idx != other.idx) {
			node_idx_unsafe_t tmp = idx.exchange(other.idx);
			node_release(tmp);
			other.idx = 0;
		}
		return *this; 
	}
	operator node_idx_unsafe_t() const { return idx; }
	bool operator==(const atomic_node_idx_t& other) const { return idx == other.idx; }
	bool operator!=(const atomic_node_idx_t& other) const { return idx != other.idx; }
	bool operator==(node_idx_unsafe_t _idx) const { return idx == _idx; }
	bool operator!=(node_idx_unsafe_t _idx) const { return idx != _idx; }
#ifdef USE_64BIT_NODES
	bool operator==(int _idx) const { return idx == _idx; }
	bool operator!=(int _idx) const { return idx != _idx; }
#endif

	node_idx_t load() const { 
		return idx.load(std::memory_order_relaxed); 
	}
	void store(node_idx_t _idx) {
		node_add_ref(_idx);
		node_release(idx.exchange(_idx));
	}
	bool compare_exchange_strong(node_idx_unsafe_t expected, node_idx_unsafe_t desired) { 
		if(idx.compare_exchange_strong(expected, desired)) {
			node_add_ref(desired);
			node_release(expected);
			return true;
		}
		return false;
	}
	bool compare_exchange_weak(node_idx_unsafe_t expected, node_idx_unsafe_t desired) { 
		if(idx.compare_exchange_weak(expected, desired)) {
			node_add_ref(desired);
			node_release(expected);
			return true;
		}
		return false;
	}
};

static node_idx_t new_node_int(long long i, int flags = 0);
static inline long long get_node_int(node_idx_t idx);

static bool node_sym_eq(node_idx_unsafe_t n1i, node_idx_unsafe_t n2i);
static bool node_eq(node_idx_t n1i, node_idx_t n2i);
static bool node_lt(node_idx_t n1i, node_idx_t n2i);
static bool node_lte(node_idx_t n1i, node_idx_t n2i);

size_t jo_hash_value(node_idx_t n);

struct jo_persistent_list;
typedef jo_persistent_list list_t;
typedef jo_alloc_t<list_t> list_alloc_t;
list_alloc_t list_alloc;
typedef jo_shared_ptr_t<list_t> list_ptr_t;

template<typename... A>
static list_ptr_t new_list(A... args) { return list_ptr_t(list_alloc.emplace(args...)); }

struct jo_persistent_list_node_t;
typedef jo_persistent_list_node_t list_node_t;
typedef jo_alloc_t<list_node_t> list_node_alloc_t;
list_node_alloc_t list_node_alloc;
typedef jo_shared_ptr_t<list_node_t> list_node_ptr_t;

template<typename... A>
static list_node_ptr_t new_list_node(A... args) { return list_node_ptr_t(list_node_alloc.emplace(args...)); }

struct jo_persistent_list_node_t {
    typedef node_idx_t T;
    T value;
    list_node_ptr_t next;
    jo_persistent_list_node_t() : value(), next() {}
    jo_persistent_list_node_t(const T &value, list_node_ptr_t next) : value(value), next(next) {}
    jo_persistent_list_node_t(const jo_persistent_list_node_t &other) : value(other.value), next(other.next) {}
    jo_persistent_list_node_t &operator=(const list_node_ptr_t &other) {
        value = other->value;
        next = other->next;
        return *this;
    }
    jo_persistent_list_node_t &operator=(list_node_ptr_t &&other) {
        value = other->value;
        next = other->next;
        return *this;
    }
    bool operator==(const list_node_ptr_t &other) const { return value == other->value && next == other->next; }
    bool operator!=(const list_node_ptr_t &other) const { return !(*this == other); }
};

// A persistent (non-destructive) linked list implementation.
struct jo_persistent_list : jo_object {
    typedef node_idx_t T;

    list_node_ptr_t head;
    list_node_ptr_t tail;
    size_t length;

    jo_persistent_list() : head(nullptr), tail(nullptr), length(0) {}
    jo_persistent_list(const jo_persistent_list &other) : head(other.head), tail(other.tail), length(other.length) {}
    jo_persistent_list &operator=(const jo_persistent_list &other) {
        head = other.head;
        tail = other.tail;
        length = other.length;
        return *this;
    }

    ~jo_persistent_list() {}

    list_ptr_t cons(const T &value) const {
        list_ptr_t copy = new_list(*this);
        copy->head = new_list_node(value, copy->head);
        if(!copy->tail) {
            copy->tail = copy->head;
        }
        copy->length++;
        return copy;
    }

    auto cons_inplace(const T &value) {
        head = new_list_node(value, head);
        if(!tail) {
            tail = head;
        }
        length++;
        return this;
    }

    // makes a new list in reverse order of the current one
    list_ptr_t reverse() const {
        list_ptr_t copy = new_list();
        list_node_ptr_t cur = head;
        while(cur) {
            copy->head = new_list_node(cur->value, copy->head);
            if(!copy->tail) {
                copy->tail = copy->head;
            }
            cur = cur->next;
        }
        copy->length = length;
        return copy;
    }

    list_ptr_t clone() const {
        list_ptr_t copy = new_list();
        list_node_ptr_t cur = head;
        list_node_ptr_t cur_copy = nullptr;
        list_node_ptr_t prev = nullptr;
        copy->head = nullptr;
        while(cur) {
            cur_copy = new_list_node(cur->value, nullptr);
            if(prev) {
                prev->next = cur_copy;
            } else {
                copy->head = cur_copy;
            }
            prev = cur_copy;
            cur = cur->next;
        }
        copy->tail = prev;
        copy->length = length;
        return copy;
    }

    // Clone up to and including the given index and link the rest of the list to the new list.
    list_ptr_t clone(int depth) const {
        list_ptr_t copy = new_list();
        list_node_ptr_t cur = head;
        list_node_ptr_t cur_copy = nullptr;
        list_node_ptr_t prev = nullptr;
        while(cur && depth >= 0) {
            cur_copy = new_list_node(cur->value, nullptr);
            if(prev) {
                prev->next = cur_copy;
            } else {
                copy->head = cur_copy;
            }
            prev = cur_copy;
            cur = cur->next;
            depth--;
        }
        if(cur) {
            prev->next = cur;
            copy->tail = tail;
        } else {
            copy->tail = prev;
        }
        copy->length = length;
        return copy;
    }

    list_ptr_t conj(const jo_persistent_list &other) const {
        list_ptr_t copy = clone();
        if(copy->tail) {
            copy->tail->next = other.head;
        } else {
            copy->head = other.head;
        }
        copy->tail = other.tail;
        copy->length += other.length;
        return copy;
    }

    auto conj_inplace(const jo_persistent_list &other) {
        if(tail) {
            tail->next = other.head;
        } else {
            head = other.head;
        }
        tail = other.tail;
        length += other.length;
        return this;
    }

    // conj a value
    list_ptr_t conj(const T &value) const {
        list_ptr_t copy = clone();
        if(copy->tail) {
            copy->tail->next = new_list_node(value, nullptr);
            copy->tail = copy->tail->next;
        } else {
            copy->head = new_list_node(value, nullptr);
            copy->tail = copy->head;
        }
        copy->length++;
        return copy;
    }

    auto conj_inplace(const T &value) {
        if(tail) {
            tail->next = new_list_node(value, nullptr);
            tail = tail->next;
        } else {
            head = new_list_node(value, nullptr);
            tail = head;
        }
        length++;
        return this;
    }

    auto disj_inplace(const jo_persistent_list &other) {
        list_node_ptr_t cur = head;
        list_node_ptr_t prev = nullptr;
        while(cur) {
            if(other.contains(cur->value)) {
                if(prev) {
                    prev->next = cur->next;
                } else {
                    head = cur->next;
                }
                length--;
                if(!cur->next) {
                    tail = prev;
                }
            } else {
                prev = cur;
            }
            cur = cur->next;
        }
        return this;
    }

    list_ptr_t disj(const jo_persistent_list &other) const {
        list_ptr_t copy = clone();
        copy->disj_inplace(other);
        return copy;
    }

    // disj with lambda
    template<typename F>
    auto disj_inplace(F f) {
        list_node_ptr_t cur = head;
        list_node_ptr_t prev = nullptr;
        while(cur) {
            if(f(cur->value)) {
                if(prev) {
                    prev->next = cur->next;
                } else {
                    head = cur->next;
                }
                length--;
                if(!cur->next) {
                    tail = prev;
                }
            } else {
                prev = cur;
            }
            cur = cur->next;
        }
        return this;
    }

    template<typename F>
    list_ptr_t disj(F f) const {
        list_ptr_t copy = clone();
        copy->disj_inplace(f);
        return copy;
    }

    list_ptr_t assoc(size_t index, const T &value) const {
        list_ptr_t copy = new_list();
        list_node_ptr_t cur = head;
        list_node_ptr_t cur_copy = nullptr;
        list_node_ptr_t prev = nullptr;
        while(cur && index >= 0) {
            cur_copy = new_list_node(cur->value, nullptr);
            if(prev) {
                prev->next = cur_copy;
            } else {
                copy->head = cur_copy;
            }
            prev = cur_copy;
            cur = cur->next;
            index--;
        }
        prev->value = value;
        if(cur) {
            prev->next = cur;
            copy->tail = tail;
        } else {
            copy->tail = prev;
        }
        copy->length = length;
        return copy;
    }

    list_ptr_t erase_node(list_node_ptr_t at) const {
        list_ptr_t copy = new_list();
        list_node_ptr_t cur = head;
        list_node_ptr_t cur_copy = nullptr;
        list_node_ptr_t prev = nullptr;
        while(cur && cur != at) {
            cur_copy = new_list_node(cur->value, nullptr);
            if(prev) {
                prev->next = cur_copy;
            } else {
                copy->head = cur_copy;
            }
            prev = cur_copy;
            cur = cur->next;
        }
        if(cur) {
            if(prev) {
                prev->next = cur->next;
            } else {
                copy->head = cur->next;
            }
            copy->tail = tail;
        } else {
            copy->tail = prev;
        }
        copy->length = length-1;
        return copy;
    }

    // pop off value from front
    list_ptr_t pop() const {
        list_ptr_t copy = new_list(*this);
        if(head) {
            copy->head = head->next;
            if(!copy->head) {
                copy->tail = nullptr;
            }
            copy->length = length - 1;
        }
        return copy;
    }

    const T &nth(int index) const {
        list_node_ptr_t cur = head;
        while(cur && index > 0) {
            cur = cur->next;
            index--;
        }
        return cur->value;
    }

    const T &operator[](int index) const {
        return nth(index);
    }

    list_ptr_t rest() const {
        list_ptr_t copy = new_list();
        if(head) {
            copy->head = head->next;
            copy->tail = tail;
            copy->length = length - 1;
        }
        return copy;
    }

    list_ptr_t first() const {
        list_ptr_t copy = new_list();
        if(head) {
            copy->head = new_list_node(head->value, nullptr);
            copy->tail = copy->head;
            copy->length = 1;
        }
        return copy;
    }

    T first_value() const {
        if(!head) {
            return T();
        }
        return head->value;
    }

    T second_value() const {
        if(!head || !head->next) {
            return T();
        }
        return head->next->value;
    }

    T third_value() const {
        if(!head || !head->next || !head->next->next) {
            return T();
        }
        return head->next->next->value;
    }

    T last_value() const {
        if(!tail) {
            return T();
        }
        return tail->value;
    }

    list_ptr_t drop(int index) const {
        list_ptr_t copy = new_list();
        list_node_ptr_t cur = head;
        while(cur && --index > 0) {
            cur = cur->next;
        }
        if(cur) {
            copy->head = cur->next;
            copy->tail = tail;
            copy->length = length - index;
        }
        return copy;
    }

    // push_back 
    auto push_back_inplace(const T &value) {
        if(!head) {
            head = new_list_node(value, nullptr);
            tail = head;
        } else {
            tail->next = new_list_node(value, nullptr);
            tail = tail->next;
        }
        length++;
        return this;
    }

    list_ptr_t push_back(const T &value) const {
        list_ptr_t copy = clone();
        copy->push_back_inplace(value);
        return copy;
    }

    // push_front inplace
    auto push_front_inplace(const T &value) {
        if(!head) {
            head = new_list_node(value, nullptr);
            tail = head;
        } else {
            list_node_ptr_t new_head = new_list_node(value, head);
            head = new_head;
        }
        length++;
        return this;
    }

    list_ptr_t push_front(const T &value) const {
        list_ptr_t copy = new_list(*this);
        copy->push_front_inplace(value);
        return copy;
    }

    auto push_front_inplace(const T &value1, const T &value2) {
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    list_ptr_t push_front(const T &value1, const T &value2) const {
        list_ptr_t copy = new_list(*this);
        copy->push_front_inplace(value1, value2);
        return copy;
    }

    auto push_front_inplace(const T &value1, const T &value2, const T &value3) {
        push_front_inplace(value3);
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    list_ptr_t push_front(const T &value1, const T &value2, const T &value3) const {
        list_ptr_t copy = new_list(*this);
        copy->push_front_inplace(value1, value2, value3);
        return copy;
    }

    auto push_front_inplace(const T &value1, const T &value2, const T &value3, const T &value4) {
        push_front_inplace(value4);
        push_front_inplace(value3);
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    list_ptr_t push_front(const T &value1, const T &value2, const T &value3, const T &value4) const {
        list_ptr_t copy = new_list(*this);
        copy->push_front_inplace(value1, value2, value3, value4);
        return copy;
    }

    auto push_front_inplace(const T &value1, const T &value2, const T &value3, const T &value4, const T &value5) {
        push_front_inplace(value5);
        push_front_inplace(value4);
        push_front_inplace(value3);
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    list_ptr_t push_front(const T &value1, const T &value2, const T &value3, const T &value4, const T &value5) const {
        list_ptr_t copy = new_list(*this);
        copy->push_front_inplace(value1, value2, value3, value4, value5);
        return copy;
    }

    auto push_front_inplace(const T &value1, const T &value2, const T &value3, const T &value4, const T &value5, const T &value6) {
        push_front_inplace(value6);
        push_front_inplace(value5);
        push_front_inplace(value4);
        push_front_inplace(value3);
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    list_ptr_t push_front(const T &value1, const T &value2, const T &value3, const T &value4, const T &value5, const T &value6) const {
        list_ptr_t copy = new_list(*this);
        copy->push_front_inplace(value1, value2, value3, value4, value5, value6);
        return copy;
    }

    list_ptr_t push_front(list_ptr_t other) const {
        list_ptr_t copy = new_list(*this);
        list_node_ptr_t cur = other->head;
        while(cur) {
            copy->push_front_inplace(cur->value);
            cur = cur->next;
        }
        return copy;
    }

    auto pop_front_inplace() {
        if(head) {
            head = head->next;
            if(!head) {
                tail = nullptr;
            }
            length--;
        }
        return this;
    }

    auto pop_front_inplace(size_t num) {
        while(num-- > 0 && head) {
            head = head->next;
            if(!head) {
                tail = nullptr;
            }
            length--;
        }
        return this;
    }

    list_ptr_t pop_front() const {
        list_ptr_t copy = new_list(*this);
        copy->pop_front_inplace();
        return copy;
    }

    list_ptr_t pop_front(size_t num) const {
        list_ptr_t copy = new_list(*this);
        copy->pop_front_inplace(num);
        return copy;
    }

    list_ptr_t subvec(int start, int end) const {
        list_node_ptr_t cur = head;
        while(cur && start > 0) {
            cur = cur->next;
            start--;
        }
        list_ptr_t copy = new_list();
        while(cur && end > start) {
            copy->push_back_inplace(cur->value);
            cur = cur->next;
            end--;
        }
        return copy;
    }

    // pop_back
    auto pop_back_inplace() {
        if(head) {
            if(head == tail) {
                head = tail = nullptr;
            } else {
                list_node_ptr_t cur = head;
                while(cur->next != tail) {
                    cur = cur->next;
                }
                tail = cur;
                tail->next = nullptr;
            }
            length--;
        }
        return this;
    }

    list_ptr_t pop_back() const {
        list_ptr_t copy = new_list(*this);
        copy->pop_back_inplace();
        return copy;
    }

    bool contains(const T &value) const {
        list_node_ptr_t cur = head;
        while(cur) {
            if(cur->value == value) {
                return true;
            }
            cur = cur->next;
        }
        return false;
    }

    // contains with lambda for comparison
    template<typename F>
    bool contains(const F &f) const {
        list_node_ptr_t cur = head;
        while(cur) {
            if(f(cur->value)) {
                return true;
            }
            cur = cur->next;
        }
        return false;
    }

    list_ptr_t erase(const T &value) {
        list_node_ptr_t cur = head;
        list_node_ptr_t prev = nullptr;
        while(cur) {
            if(cur->value == value) {
                return erase_node(cur);
            }
            prev = cur;
            cur = cur->next;
        }
        return this;
    }

    list_ptr_t take(int N) const {
        list_ptr_t copy = new_list();
        list_node_ptr_t cur = head;
        while(cur && N > 0) {
            copy->push_back_inplace(cur->value);
            cur = cur->next;
            N--;
        }
        return copy;
    }

    list_ptr_t take_last(int N) const {
        return drop(length - N);
    }

    // return a random permutation of the elements of the list
    list_ptr_t shuffle() const {
        // convert to jo_vector
        jo_vector<T> v;
        list_node_ptr_t cur = head;
        while(cur) {
            v.push_back(cur->value);
            cur = cur->next;
        }
        // shuffle
        jo_random_shuffle(v.begin(), v.end());
        // convert back to jo_persistent_list
        list_ptr_t copy = new_list();
        for(int i = 0; i < v.size(); i++) {
            copy->push_back_inplace(v[i]);
        }
        return copy;
    }

    // return items with a random probability of p
    list_ptr_t random_sample(float p) const { 
        if(p <= 0.0f) {
            return new_list();
        }
        if(p >= 1.0f) {
            return new_list(*this);
        }
        list_ptr_t copy = new_list();
        list_node_ptr_t cur = head;
        while(cur) {
            if(jo_random_float() < p) {
                copy->push_back_inplace(cur->value);
            }
            cur = cur->next;
        }
        return copy;
    }

    // iterator
    struct iterator {
        list_node_ptr_t cur;
        int index;

        iterator() : cur(nullptr), index() {}
        //iterator(list_ptr_t cur) : cur(cur), index() {}
        iterator(const iterator &other) : cur(other.cur), index(other.index) {}
        iterator(const jo_persistent_list &list) : cur(list.head), index() {}
        iterator(const list_ptr_t &list) : cur(list->head), index() {}

        iterator &operator=(const iterator &other) {
            cur = other.cur;
            index = other.index;
            return *this;
        }
        bool operator==(const iterator &other) const {
            return cur == other.cur;
        }
        bool operator!=(const iterator &other) const {
            return cur != other.cur;
        }
        operator bool() const {
            return cur;
        }
        bool has_next() const {
            return cur && cur->next;
        }
        iterator &operator++() {
            if(cur) {
                cur = cur->next;
                ++index;
            }
            return *this;
        }
        iterator operator++(int) {
            iterator copy = *this;
            if(cur) {
                cur = cur->next;
                ++index;
            }
            return copy;
        }
        iterator operator+=(int n) {
            while(cur && n > 0) {
                cur = cur->next;
                n--;
                ++index;
            }
            return *this;
        }
        iterator operator+(int n) const {
            iterator copy = *this;
            copy += n;
            return copy;
        }
        T &operator*() {
            if(cur) {
                return cur->value;
            }
            static T dummy;
            return dummy;
        }
        T *operator->() {
            if(cur) {
                return &cur->value;
            }
            static T dummy;
            return &dummy;
        }
        const T &operator*() const {
            if(cur) {
                return cur->value;
            }
            static T dummy;
            return dummy;
        }
        const T *operator->() const {
            if(cur) {
                return &cur->value;
            }
            static T dummy;
            return &dummy;
        }

    };

    list_ptr_t rest(const iterator &it) const {
        list_ptr_t copy = new_list();
        copy->head = it.cur;
        copy->tail = tail;
        copy->length = length - it.index;
        return copy;
    }

    const T &front() const {
        return head->value;
    }

    const T &back() const {
        return tail->value;
    }

    size_t size() const {
        return length;
    }

    bool empty() const {
        return !head;
    }

    void clear() {
        head = nullptr;
        tail = nullptr;
        length = 0;
    }
};


template<typename T>
struct jo_persistent_vector_node_t {
    typedef jo_shared_ptr_t<jo_persistent_vector_node_t> node_shared_ptr;

    node_shared_ptr children[32];
    T elements[32];

    jo_persistent_vector_node_t() : children() {}

    jo_persistent_vector_node_t(const node_shared_ptr &other) {
        if(other.ptr) {
            for (int i = 0; i < 32; ++i) {
                children[i] = other->children[i];
                elements[i] = other->elements[i];
            }
        }
    }
};

// Persistent Vector implementation (vector that supports versioning)
// For use in purely functional languages
template<typename T>
struct jo_persistent_vector : jo_object {
    typedef jo_persistent_vector_node_t<T> vector_node_t;
    typedef jo_persistent_vector<T> vector_t;

    typedef jo_shared_ptr_t<vector_node_t> node_shared_ptr;
    typedef jo_shared_ptr_t<vector_t> shared_ptr;

    typedef jo_alloc_t<vector_node_t> vector_node_alloc_t;
    typedef jo_shared_ptr_t<vector_node_t> vector_node_ptr_t;

    typedef jo_alloc_t<vector_t> vector_alloc_t;
    typedef jo_shared_ptr_t<vector_t> vector_ptr_t;

    static vector_node_alloc_t alloc_node;
    static vector_alloc_t alloc;

    template<typename...A> node_shared_ptr new_node(A...args) const { return node_shared_ptr(alloc_node.emplace(args...)); }
    template<typename...A> shared_ptr new_vector(A... args) const { return shared_ptr(alloc.emplace(args...)); }

    node_shared_ptr head;
    node_shared_ptr tail;
    long long head_offset;
    long long length;
    int tail_length;
    int depth;

    jo_persistent_vector() {
        head = nullptr;
        tail = new_node();
        head_offset = 0;
        tail_length = 0;
        length = 0;
        depth = 0;
    }

    jo_persistent_vector(long long initial_size) {
        head = nullptr;
        tail = new_node();
        head_offset = 0;
        tail_length = 0;
        length = 0;
        depth = 0;
        if(length < initial_size-31) {
            tail_length = 32;
            length += 32;
            while(length < initial_size-31) {
                append_tail();
                tail_length = 32;
                length += 32;
            }
        }
        while(length < initial_size) {
            push_back_inplace(T());
        }
    }

    jo_persistent_vector(const jo_persistent_vector &other) {
        head = other.head;
        tail = other.tail;
        head_offset = other.head_offset;
        tail_length = other.tail_length;
        length = other.length;
        depth = other.depth;
    }

    shared_ptr clone() const { return new_vector(*this); }

    void append_tail() {
        size_t tail_offset = length + head_offset - tail_length;
        size_t shift = 5 * (depth + 1);
        size_t max_size = 1 << shift;

        // check for root overflow, and if so expand tree by 1 level
        if(length >= max_size) {
            node_shared_ptr new_root = new_node();
            new_root->children[0] = head;
            head = new_root;
            ++depth;
            shift = 5 * (depth + 1);
        } else {
            head = new_node(head);
        }

        // Set up our tree traversal. We subtract 5 from level each time
        // in order to get all the way down to the level above where we want to
        // insert this tail jo_persistent_vector_node_t.
        node_shared_ptr cur = NULL;
        node_shared_ptr prev = head;
        size_t key = tail_offset;
        for(size_t level = shift; level > 5; level -= 5) {
            size_t index = (key >> level) & 31;
            // we are at the end of our tree, insert tail jo_persistent_vector_node_t
            cur = prev->children[index];
            if(!cur) {
                cur = new_node();
            } else {
                cur = new_node(cur);
            }
            prev->children[index] = cur;
            prev = cur;
        }
        prev->children[(key >> 5) & 31] = tail;

        // Make our new tail
        tail = new_node();
        tail_length = 0;
    }

    shared_ptr append(const T &value) const {
        // Create a copy of our root jo_persistent_vector_node_t from which we will base our append
        shared_ptr copy = clone();

        // do we have space?
        if(copy->tail_length >= 32) {
            copy->append_tail();
        } else {
            copy->tail = new_node(copy->tail);
        }

        copy->tail->elements[copy->tail_length++] = value;
        copy->length++;
        return copy;
    }

    // append inplace
    void append_inplace(const T &value) {
        // do we have space?
        if(tail_length >= 32) {
            append_tail();
        }

        tail->elements[tail_length++] = value;
        length++;
    }

    shared_ptr assoc(size_t index, const T &value) const {
        if(index >= length) {
            return append(value);
        }

        index += head_offset;

        // Create a copy of our root jo_persistent_vector_node_t from which we will base our append
        shared_ptr copy = clone();

        size_t tail_offset = length + head_offset - tail_length;
        if(index < tail_offset) {
            copy->head = new_node(copy->head);

            node_shared_ptr cur = copy->head;
            size_t i;
            switch(depth) {
                case 11: i = (index >> 60) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 10: i = (index >> 55) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 9: i = (index >> 50) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 8: i = (index >> 45) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 7: i = (index >> 40) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 6: i = (index >> 35) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 5: i = (index >> 30) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 4: i = (index >> 25) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 3: i = (index >> 20) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 2: i = (index >> 15) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 1: i = (index >> 10) & 31; cur = cur->children[i] = new_node(cur->children[i]);
            }
            i = (index >> 5) & 31; cur = cur->children[i] = new_node(cur->children[i]);
            cur->elements[index & 31] = value;
        } else {
            copy->tail = new_node(copy->tail);
            copy->tail->elements[index - tail_offset] = value;
        }

        return copy;
    }

    void assoc_inplace(size_t index, const T &value) {
        if(index >= length) {
            return append_inplace(value);
        }

        index += head_offset;

        // Create a copy of our root jo_persistent_vector_node_t from which we will base our append
        size_t tail_offset = length + head_offset - tail_length;
        if(index < tail_offset) {
            head = new_node(head);

            node_shared_ptr cur = head;
            size_t i;
            switch(depth) {
                case 11: i = (index >> 60) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 10: i = (index >> 55) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 9: i = (index >> 50) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 8: i = (index >> 45) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 7: i = (index >> 40) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 6: i = (index >> 35) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 5: i = (index >> 30) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 4: i = (index >> 25) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 3: i = (index >> 20) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 2: i = (index >> 15) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 1: i = (index >> 10) & 31; cur = cur->children[i] = new_node(cur->children[i]);
            }
            i = (index >> 5) & 31; cur = cur->children[i] = new_node(cur->children[i]);
            cur->elements[index & 31] = value;
        } else {
            tail = new_node(tail);
            tail->elements[index - tail_offset] = value;
        }
    }

    void assoc_mutate(size_t index, const T &value) {
        if(index >= length) {
            return append_inplace(value);
        }

        index += head_offset;

        size_t tail_offset = length + head_offset - tail_length;
        if(index < tail_offset) {
            node_shared_ptr cur = head;
            size_t i;
            switch(depth) {
                case 11: i = (index >> 60) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 10: i = (index >> 55) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 9: i = (index >> 50) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 8: i = (index >> 45) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 7: i = (index >> 40) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 6: i = (index >> 35) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 5: i = (index >> 30) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 4: i = (index >> 25) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 3: i = (index >> 20) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 2: i = (index >> 15) & 31; cur = cur->children[i] = new_node(cur->children[i]);
                case 1: i = (index >> 10) & 31; cur = cur->children[i] = new_node(cur->children[i]);
            }
            i = (index >> 5) & 31; cur = cur->children[i] = new_node(cur->children[i]);
            cur->elements[index & 31] = value;
        } else {
            tail->elements[index - tail_offset] = value;
        }
    }

    auto dissoc_inplace(size_t index) { return assoc_inplace(index, T()); }
    auto cons_inplace(const T &value) { return append_inplace(value); }
    auto conj_inplace(const T &value) { return append_inplace(value); }

    shared_ptr dissoc(size_t index) const { return assoc(index, T()); }
    shared_ptr set(size_t index, const T &value) const { return assoc(index, value); }
    shared_ptr cons(const T &value) const { return append(value); }
    shared_ptr conj(const T &value) const { return append(value); }
    shared_ptr disj(const T &value) const { return dissoc(value); }

    shared_ptr conj(const jo_persistent_vector &other) const {
        shared_ptr copy = clone();
        // loop through other, and append to copy
        for(size_t i = 0; i < other.length; ++i) {
            copy->append_inplace(other[i]);
        }
        return copy;
    }

    void conj_inplace(const jo_persistent_vector &other) {
        // loop through other, and append to copy
        for(size_t i = 0; i < other.length; ++i) {
            append_inplace(other[i]);
        }
    }

    shared_ptr push_back(const T &value) const { return append(value); }
    void push_back_inplace(const T &value) { return append_inplace(value); }

    shared_ptr resize(size_t new_size) const {
        if(new_size == length) {
            return new_vector(*this);
        }

        if(new_size < length) {
            return take(new_size);
        }

        // This could be faster...
        shared_ptr copy = new_vector(*this);
        for(size_t i = length; i < new_size; ++i) {
            copy->append_inplace(T());
        }
        return copy;
    }

    shared_ptr push_front(const T &value) const {
        assert(head_offset > 0);
        shared_ptr copy = new_vector(*this);
        copy->head_offset--;
        copy->length++;
        copy->assoc_inplace(0, value);
        return copy;
    }

    void push_front_inplace(const T &value) {
        assert(head_offset > 0);
        head_offset--;
        length++;
        assoc_inplace(0, value);
    }
    
    shared_ptr pop_back() const {
        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Create a copy of our root jo_persistent_vector_node_t from which we will base our append
        shared_ptr copy = new_vector(*this);

        // Is it in the tail?
        if(length + head_offset >= tail_offset) {
            // copy the tail (since we are changing the data)
            copy->tail = new_node(copy->tail);
            copy->tail_length--;
            copy->length--;
            return copy;
        }

        // traverse duplicating the way down.
        node_shared_ptr cur = NULL;
        node_shared_ptr prev = copy->head;
        size_t key = length + head_offset - 1;
        for (size_t level = shift; level > 0; level -= 5) {
            size_t index = (key >> level) & 0x1f;
            // copy nodes as we traverse
            cur = new_node(prev->children[index]);
            prev->children[index] = cur;
            prev = cur;
        }
        prev->elements[key & 0x1f] = T();
        copy->tail_length--;
        copy->length--;
        return copy;
    }

    void pop_back_inplace() {
        if(length == 0) {
            return;
        }

        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Is it in the tail?
        if(length + head_offset >= tail_offset) {
            // copy the tail (since we are changing the data)
            tail = new_node(tail);
            tail_length--;
            length--;
            return;
        }

        // traverse duplicating the way down.
        node_shared_ptr cur = NULL;
        node_shared_ptr prev = head;
        size_t key = length + head_offset - 1;
        for (size_t level = shift; level > 0; level -= 5) {
            size_t index = (key >> level) & 0x1f;
            // copy nodes as we traverse
            cur = new_node(prev->children[index]);
            prev->children[index] = cur;
            prev = cur;
        }
        prev->elements[key & 31] = T();
        tail_length--;
        length--;
    }

    shared_ptr pop_front() const {
        shared_ptr copy = new_vector(*this);
        copy->head_offset++;
        copy->length--;
        return copy;
    }

    void pop_front_inplace() {
        head_offset++;
        length--;
    }

    shared_ptr rest() const { return pop_front(); }
    shared_ptr pop() const { return pop_front(); }

    shared_ptr drop(size_t n) const {
        if(n == 0) {
            return new_vector(*this);
        }
        if(n >= length) {
            return new_vector();
        }
        shared_ptr copy = new_vector(*this);
        copy->head_offset += n;
        copy->length -= n;
        return copy;
    }

    shared_ptr take(size_t n) const {
        if(n >= length) {
            return new_vector(*this);
        }
        shared_ptr copy = new_vector();
        for(size_t i = 0; i < n; ++i) {
            copy->append_inplace(nth(i));
        }
        return copy;
    }

    shared_ptr take_last(size_t n) const {
        if(n >= length) {
            return new_vector(*this);
        }
        return drop(length - n);
    }

    shared_ptr random_sample(float prob) const {
        if(prob <= 0.0f) {
            return new_vector();
        }
        if(prob >= 1.0f) {
            return new_vector(*this);
        }

        shared_ptr copy = new_vector();
        for(size_t i = 0; i < length; ++i) {
            if(jo_random_float() < prob) {
                copy->push_back_inplace(nth(i));
            }
        }
        return copy;
    }

    shared_ptr shuffle() const {
        shared_ptr copy = new_vector(*this);
        for(size_t i = 0; i < length; ++i) {
            size_t j = jo_random_int(i, length);
            T tmp = copy->nth(i);
            copy->assoc_inplace(i, copy->nth(j));
            copy->assoc_inplace(j, tmp);
        }
        return copy;
    }

    inline T &nth(long long index) { 
        // Is it in the tail?
        long long tail_offset = length - tail_length;
        if(index >= tail_offset) {
            return tail->elements[index - tail_offset];
        }

        index += head_offset;

        // traverse 
        vector_node_t *cur = head.ptr;
        switch(depth) {
            case 11: cur = cur->children[(index >> 60) & 31].ptr;
            case 10: cur = cur->children[(index >> 55) & 31].ptr;
            case 9: cur = cur->children[(index >> 50) & 31].ptr;
            case 8: cur = cur->children[(index >> 45) & 31].ptr; 
            case 7: cur = cur->children[(index >> 40) & 31].ptr;
            case 6: cur = cur->children[(index >> 35) & 31].ptr;
            case 5: cur = cur->children[(index >> 30) & 31].ptr;
            case 4: cur = cur->children[(index >> 25) & 31].ptr;
            case 3: cur = cur->children[(index >> 20) & 31].ptr;
            case 2: cur = cur->children[(index >> 15) & 31].ptr;
            case 1: cur = cur->children[(index >> 10) & 31].ptr;
        }
        cur = cur->children[(index >> 5) & 31].ptr;
        return cur->elements[index & 31];
    }

    inline const T &nth(long long index) const { 
        // Is it in the tail?
        long long tail_offset = length - tail_length;
        if(index >= tail_offset) {
            return tail->elements[index - tail_offset];
        }

        index += head_offset;

        // traverse
        vector_node_t *cur = head.ptr;
        switch(depth) {
            case 11: cur = cur->children[(index >> 60) & 31].ptr;
            case 10: cur = cur->children[(index >> 55) & 31].ptr;
            case 9: cur = cur->children[(index >> 50) & 31].ptr;
            case 8: cur = cur->children[(index >> 45) & 31].ptr; 
            case 7: cur = cur->children[(index >> 40) & 31].ptr;
            case 6: cur = cur->children[(index >> 35) & 31].ptr;
            case 5: cur = cur->children[(index >> 30) & 31].ptr;
            case 4: cur = cur->children[(index >> 25) & 31].ptr;
            case 3: cur = cur->children[(index >> 20) & 31].ptr;
            case 2: cur = cur->children[(index >> 15) & 31].ptr;
            case 1: cur = cur->children[(index >> 10) & 31].ptr;
        }
        cur = cur->children[(index >> 5) & 31].ptr;
        return cur->elements[index & 31];
    }

    inline T &nth_clamp(long long index) {
        index = index < 0 ? 0 : index;
        index = index > (long long)length-1 ? length-1 : index;
        return nth(index);
    }
    
    inline const T &nth_clamp(long long index) const {
        index = index < 0 ? 0 : index;
        index = index > (long long)length-1 ? length-1 : index;
        return nth(index);
    }

    inline const T &first_value() const { return nth(0); }
    inline size_t size() const { return length; }
    inline bool empty() const { return length == 0; }

    shared_ptr reverse() {
        shared_ptr copy = new_vector();
        for(long long i = length-1; i >= 0; --i) {
            copy->push_back_inplace(nth(i));
        }
        return copy;
    }

    // find with lambda for comparison
    template<typename F>
    size_t find(const F &f) const {
        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Is it in the tail?
        if(tail_length > 0) {
            for(size_t i = 0; i < tail_length; ++i) {
                if(f(tail->elements[i])) {
                    return tail_offset + i;
                }
            }
        }

        for(size_t i = 0; i < tail_offset; ++i) {
            if(f(nth(i))) {
                return i;
            }
        }

        return jo_npos;
    }

    // contains with lambda for comparison
    template<typename F>
    bool contains(const F &f) const {
        return find(f) != jo_npos;
    }

    shared_ptr subvec(size_t start, size_t end) const {
        if(start == 0) {
            return take(end);
        }
        if(end >= length) {
            return drop(start);
        }
        return drop(start)->take(end - start);
    }

    void print() const {
        printf("[");
        for(size_t i = 0; i < size(); ++i) {
            if(i > 0) {
                printf(", ");
            }
            printf("%d", (*this)[i]);
        }
        printf("]");
    }

    // iterator
    class iterator {
    public:
        iterator() : vec(NULL), index(0) {}
        iterator(const jo_persistent_vector *vec, size_t index) : vec(vec), index(index) {}
        iterator(const iterator &other) : vec(other.vec), index(other.index) {}
        iterator &operator=(const iterator &other) {
            vec = other.vec;
            index = other.index;
            return *this;
        }
        iterator &operator++() {
            ++index;
            return *this;
        }
        iterator operator++(int) {
            iterator copy(*this);
            ++index;
            return copy;
        }
        bool operator==(const iterator &other) const { return vec == other.vec && index == other.index; }
        bool operator!=(const iterator &other) const { return vec != other.vec || index != other.index; }
        operator bool() const { return index < vec->size(); }
        const T &operator*() const { return vec->nth(index); }
        const T *operator->() const { return &(vec->nth(index)); }
        iterator operator+(size_t offset) const { return iterator(vec, index + offset); }
        iterator operator-(size_t offset) const { return iterator(vec, index - offset); }
        iterator &operator+=(size_t offset) { index += offset; return *this; }
        iterator &operator-=(size_t offset) { index -= offset; return *this; }
        size_t operator-(const iterator &other) const { return index - other.index; }
        size_t operator-(const iterator &other) { return index - other.index; }

    private:
        const jo_persistent_vector *vec;
        size_t index;
    };

    iterator begin() const { return iterator(this, 0); }
    iterator end() const { return iterator(this, size()); }
    T &back() { return tail->elements[tail_length - 1]; }
    const T &back() const { return tail->elements[tail_length - 1]; }
    T &front() { return nth(0); }
    const T &front() const { return nth(0); }
    T &first_value() { return nth(0); }
    T &last_value() { return back(); }
};

typedef jo_persistent_vector<node_idx_t> vector_t;
template<> vector_t::vector_node_alloc_t vector_t::alloc_node = vector_t::vector_node_alloc_t();
template<> vector_t::vector_alloc_t vector_t::alloc = vector_t::vector_alloc_t();
typedef jo_shared_ptr_t<vector_t> vector_ptr_t;
template<typename...A> jo_shared_ptr_t<vector_t> new_vector(A... args) { return jo_shared_ptr_t<vector_t>(vector_t::alloc.emplace(args...)); }

template<typename T>
struct jo_persistent_matrix_node_t {
    typedef jo_persistent_matrix_node_t<T> matrix_node_t;
    typedef jo_shared_ptr_t<matrix_node_t> node_shared_ptr;

    // block size is 8x8 = 64 pixels
    node_shared_ptr children[64];
    T elements[64];

    jo_persistent_matrix_node_t() : children(), elements() {}
    
    ~jo_persistent_matrix_node_t() {
        for(int i = 0; i < 64; ++i) {
            children[i] = nullptr;
            if(!std::is_pod<T>::value) {
                elements[i] = std::move(T());
            }
        }
    }

    jo_persistent_matrix_node_t(const node_shared_ptr &other) : children(), elements() {
        if(other.ptr) {
            for (int i = 0; i < 64; ++i) {
                children[i] = other->children[i];
                elements[i] = other->elements[i];
            }
        }
    }
};

// A persistent matrix structure. broken up into a tree of 8x8 blocks. Like jo_persistent_vector, but 2D
template<typename T>
struct jo_persistent_matrix : jo_object {
    typedef jo_persistent_matrix_node_t<T> matrix_node_t;
    typedef jo_persistent_matrix<T> matrix_t;

    typedef jo_shared_ptr_t<matrix_node_t> node_shared_ptr;
    typedef jo_shared_ptr_t<matrix_t> shared_ptr;

    typedef jo_alloc_t<matrix_node_t> matrix_node_alloc_t;
    typedef jo_shared_ptr_t<matrix_node_t> matrix_node_ptr_t;

    typedef jo_alloc_t<matrix_t> matrix_alloc_t;
    typedef jo_shared_ptr_t<matrix_t> matrix_ptr_t;

    static matrix_node_alloc_t alloc_node;
    static matrix_alloc_t alloc;

    template<typename...A> node_shared_ptr new_node(A...args) const { return node_shared_ptr(alloc_node.emplace(args...)); }
    template<typename...A> shared_ptr new_matrix(A... args) const { return shared_ptr(alloc.emplace(args...)); }

    // The head of the matrix tree
    node_shared_ptr head;
    // Dimensions of the matrix
    size_t width,height;
    // Depth of the tree
    size_t depth;

    jo_persistent_matrix() : head(), width(0), height(0), depth(0) {}

    jo_persistent_matrix(size_t w, size_t h) {
        width = w;
        height = h;
        depth = (size_t)ceil(log2(jo_max(w, h))) / 3; // / 3 cause its 8x8
        head = new_node();
    }

    jo_persistent_matrix(const jo_persistent_matrix &other) {
        width = other.width;
        height = other.height;
        depth = other.depth;
        head = other.head;
    }

    shared_ptr clone() const { return new_matrix(*this); }

    void set(size_t x, size_t y, const T &value) {
        if(x >= width || y >= height) {
            return;
        }

        int shift = 3 * (depth + 1);

        head = new_node(head);

        node_shared_ptr cur = NULL;
        node_shared_ptr prev = head;
        for (int level = shift; level > 0; level -= 3) {
            size_t xx = (x >> level) & 7;
            size_t yy = (y >> level) & 7;
            size_t i = yy * 8 + xx;

            // copy nodes as we traverse
            cur = new_node(prev->children[i]);
            prev->children[i] = cur;
            prev = cur;
        }
        prev->elements[(y & 7) * 8 + (x & 7)] = value;
    }

    shared_ptr set_new(size_t x, size_t y, T &value) const {
        shared_ptr copy = clone();
        copy->set(x, y, value);
        return copy;
    }

    T get(size_t x, size_t y) const {
        if(x >= width || y >= height) {
            return T();
        }
        int shift = 3 * (depth + 1);

        node_shared_ptr cur = head;

        for (int level = shift; level > 0; level -= 3) {
            size_t xx = (x >> level) & 7;
            size_t yy = (y >> level) & 7;
            size_t i = yy * 8 + xx;

            cur = cur->children[i];
            if (!cur) {
                return T();
            }
        }
        return cur->elements[(y & 7) * 8 + (x & 7)];
    }

    void clear() { head = nullptr; }

    T *c_array() {
        T *arr = new T[width * height];
        // TODO: could be MUCH faster
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                arr[y * width + x] = get(x, y);
            }
        }
        return arr;
    }
};

typedef jo_persistent_matrix<node_idx_t> matrix_t;
template<> matrix_t::matrix_node_alloc_t matrix_t::alloc_node = matrix_t::matrix_node_alloc_t();
template<> matrix_t::matrix_alloc_t matrix_t::alloc = matrix_t::matrix_alloc_t();
typedef jo_shared_ptr_t<matrix_t> matrix_ptr_t;
template<typename...A> jo_shared_ptr_t<matrix_t> new_matrix(A... args) { return jo_shared_ptr_t<matrix_t>(matrix_t::alloc.emplace(args...)); }

struct jo_persistent_hash_map;
typedef jo_persistent_hash_map hash_map_t;
typedef jo_alloc_t<hash_map_t> hash_map_alloc_t;
hash_map_alloc_t hash_map_alloc;
typedef jo_shared_ptr_t<hash_map_t> hash_map_ptr_t;

template<typename... A>
static hash_map_ptr_t new_hash_map(A... args) { return hash_map_ptr_t(hash_map_alloc.emplace(args...)); }

typedef jo_tuple<node_idx_t, node_idx_t, bool> jo_persistent_hash_map_entry_t;
typedef jo_persistent_vector_node_t<jo_persistent_hash_map_entry_t> hash_map_node_t;
typedef jo_alloc_t<jo_persistent_hash_map_entry_t> hash_map_node_alloc_t;
hash_map_node_alloc_t hash_map_node_alloc;
typedef jo_shared_ptr_t<hash_map_node_t> hash_map_node_ptr_t;

//template<typename T, typename...A> jo_shared_ptr_t<T> new_vector_node(A... args);
//template<typename T, typename...A> jo_shared_ptr_t<hash_map_node_t> new_vector_node(A...args) { return jo_shared_ptr_t<hash_map_node_t>(vector_node_alloc.emplace(args...)); }

template<> 
jo_persistent_vector<jo_persistent_hash_map_entry_t>::vector_node_alloc_t 
    jo_persistent_vector<jo_persistent_hash_map_entry_t>::alloc_node 
    = jo_persistent_vector<jo_persistent_hash_map_entry_t>::vector_node_alloc_t();
template<> 
jo_persistent_vector<jo_persistent_hash_map_entry_t>::vector_alloc_t 
    jo_persistent_vector<jo_persistent_hash_map_entry_t>::alloc
    = jo_persistent_vector<jo_persistent_hash_map_entry_t>::vector_alloc_t();

// jo_persistent_hash_map is a persistent hash map
// that uses a persistent vector as the underlying data structure
// for the map.
struct jo_persistent_hash_map : jo_object {
    typedef node_idx_t K;
    typedef node_idx_t V;
    // the way this works is that you make your hash value of K and
    // then use that as an index into the vector.
    typedef jo_persistent_hash_map_entry_t entry_t;

    typedef jo_persistent_vector<entry_t> vector_t;
    typedef jo_alloc_t<vector_t> alloc_t;
    typedef jo_shared_ptr_t<vector_t> ptr_t;

    static alloc_t alloc;

    template<typename...A> ptr_t new_vec(A... args) const { return ptr_t(alloc.emplace(args...)); }

    // vec is used to store the keys and values
    ptr_t vec;
    // number of entries actually used in the hash table
    size_t length;

    jo_persistent_hash_map() : vec(new_vec(32)), length() {
        assert(vec->length == 32);
    }
    jo_persistent_hash_map(const jo_persistent_hash_map &other) : vec(other.vec), length(other.length) {}
    jo_persistent_hash_map &operator=(const jo_persistent_hash_map &other) {
        vec = other.vec;
        length = other.length;
        return *this;
    }

    size_t size() const { return length; }
    bool empty() const { return !length; }

    // iterator
    class iterator {
        typename jo_persistent_vector< entry_t >::iterator cur;
    public:
        iterator(const typename jo_persistent_vector< entry_t >::iterator &it) : cur(it) {
            // find first non-erased entry
            while(cur && !cur->third) {
                cur++;
            }
        }
        iterator() : cur() {}
        iterator &operator++() {
            if(cur) do {
                cur++;
            } while(cur && !cur->third);
            return *this;
        }
        iterator operator++(int) {
            iterator ret = *this;
            if(cur) do {
                cur++;
            } while(cur && !cur->third);
            return ret;
        }
        bool operator==(const iterator &other) const { return cur == other.cur; }
        bool operator!=(const iterator &other) const { return cur != other.cur; }
        const entry_t &operator*() const { return *cur; }
        const entry_t *operator->() const { return &*cur; }
        operator bool() const { return cur; }
        iterator operator+(int i) const {
            iterator ret = *this;
            ret.cur += i;
            return ret;
        }
    };

    iterator begin() { return iterator(vec->begin()); }
    iterator begin() const { return iterator(vec->begin()); }
    iterator end() { return iterator(vec->end()); }
    iterator end() const { return iterator(vec->end()); }

    hash_map_ptr_t resize(size_t new_size) const {
        hash_map_ptr_t copy = new_hash_map();
        copy->vec = new_vec(new_size);
        for(iterator it = begin(); it; ++it) {
            int index = jo_hash_value(it->first) % new_size;
            while(copy->vec->nth(index).third) {
                index = (index + 1) % new_size;
            }
            copy->vec->assoc_inplace(index, entry_t(it->first, it->second, true));
        }
        return copy;
    }

    auto resize_inplace(size_t new_size) {
        ptr_t nv = new_vec(new_size);
        for(iterator it = begin(); it; ++it) {
            auto entry = *it;
            int index = jo_hash_value(entry.first) % new_size;
            while(nv->nth(index).third) {
                index = (index + 1) % new_size;
            }
            nv->assoc_inplace(index, entry_t(entry.first, entry.second, true));
        }
        vec = nv;
        return this;
    }

    // assoc with lambda for equality
    template<typename F>
    hash_map_ptr_t assoc(const K &key, const V &value, F eq) const {
        hash_map_ptr_t copy = new_hash_map(*this);
        if(copy->vec->size() - copy->length < copy->vec->size() / 8) {
            copy->resize_inplace(copy->vec->size() * 2);
        }
        int index = jo_hash_value(key) % copy->vec->size();
        entry_t e = copy->vec->nth(index);
        while(e.third) {
            if(eq(e.first, key)) {
                copy->vec = copy->vec->assoc(index, entry_t(key, value, true));
                return copy;
            }
            index = (index + 1) % copy->vec->size();
            e = copy->vec->nth(index);
        } 
        copy->vec = copy->vec->assoc(index, entry_t(key, value, true));
        ++copy->length;
        return copy;
    }

    // assoc with lambda for equality
    template<typename F>
    auto assoc_inplace(const K &key, const V &value, F eq) {
        if(vec->size() - length < vec->size() / 8) {
            resize_inplace(vec->size() * 2);
        }
        int index = jo_hash_value(key) % vec->size();
        entry_t e = vec->nth(index);
        while(e.third) {
            if(eq(e.first, key)) {
                vec->assoc_inplace(index, entry_t(key, value, true));
                return this;
            }
            index = (index + 1) % vec->size();
            e = vec->nth(index);
        } 
        vec->assoc_inplace(index, entry_t(key, value, true));
        ++length;
        return this;
    }

    // dissoc with lambda for equality
    template<typename F>
    hash_map_ptr_t dissoc(const K &key, F eq) const {
        hash_map_ptr_t copy = new_hash_map(*this);
        int index = jo_hash_value(key) % copy->vec->size();
        // TODO: nth twice? optimization pls
        entry_t e = copy->vec->nth(index);
        while(e.third) {
            if(eq(e.first, key)) {
                copy->vec = copy->vec->assoc(index, entry_t());
                --copy->length;
                // need to shuffle entries up to fill in the gap
                int i = index;
                int j = i;
                while(true) {
                    j = (j + 1) % copy->vec->size();
                    if(!copy->vec->nth(j).third) {
                        break;
                    }
                    entry_t next_entry = copy->vec->nth(j);
                    if(jo_hash_value(next_entry.first) % copy->vec->size() <= i) {
                        copy->vec = copy->vec->assoc(i, next_entry);
                        copy->vec = copy->vec->assoc(j, entry_t());
                        i = j;
                    }
                }
                return copy;
            }
            index = (index + 1) % copy->vec->size();
            e = copy->vec->nth(index);
        } 
        return copy;
    }

    // dissoc_inplace
    template<typename F>
    auto dissoc_inplace(const K &key, F eq) {
        int index = jo_hash_value(key) % vec->size();
        // TODO: nth twice? optimization pls
        entry_t &e = vec->nth(index);
        while(e.third) {
            if(eq(e.first, key)) {
                // TODO: optimize
                vec->assoc_inplace(index, entry_t());
                --length;
                // need to shuffle entries up to fill in the gap
                int i = index;
                int j = i;
                while(true) {
                    j = (j + 1) % vec->size();
                    if(!vec->nth(j).third) {
                        break;
                    }
                    entry_t next_entry = vec->nth(j);
                    if(jo_hash_value(next_entry.first) % vec->size() <= i) {
                        vec->assoc_inplace(i, next_entry);
                        vec->assoc_inplace(j, entry_t());
                        i = j;
                    }
                }
                return this;
            }
            index = (index + 1) % vec->size();
            e = vec->nth(index);
        }
        return this;
    }

    // find using lambda
    template<typename F>
    entry_t find(const K &key, const F &f) const {
        if(vec->size() == 0) {
            return entry_t();
        }
        size_t index = jo_hash_value(key) % vec->size();
        entry_t e = vec->nth(index);
        while(e.third) {
            if(f(e.first, key)) {
                return e;
            }
            index = (index + 1) % vec->size();
            e = vec->nth(index);
        };
        return entry_t();
    }
     
    // contains using lambda
    template<typename F>
    bool contains(const K &key, const F &f) {
        return find(key, f).third;
    }

    template<typename F>
    V get(const K &key, const F &f) const {
        if(vec->size() == 0) {
            return V();
        }
        size_t index = jo_hash_value(key) % vec->size();
        entry_t e = vec->nth(index);
        while(e.third) {
            if(f(e.first, key)) {
                return e.second;
            }
            index = (index + 1) % vec->size();
            e = vec->nth(index);
        } 
        return V();
    }

    // conj with lambda
    template<typename F>
    hash_map_ptr_t conj(hash_map_ptr_t other, F eq) const {
        hash_map_ptr_t copy = new_hash_map(*this);
        for(auto it = other->vec->begin(); it; ++it) {
            if(it->third) {
                copy = copy->assoc(it->first, it->second, eq);
            }
        }
        return copy;
    }

    entry_t first() const {
        for(auto it = this->vec->begin(); it; ++it) {
            if(it->third) {
                return *it;
            }
        }
        return entry_t();
    }

    entry_t second() const {
        for(auto it = this->vec->begin(); it; ++it) {
            if(it->third) {
                for(++it; it; ++it) {
                    if(it->third) {
                        return *it;
                    }
                }
            }
        }
        return entry_t();
    }

    V first_value() const {
        for(auto it = vec->begin(); it; ++it) {
            if(it->third) {
                return it->second;
            }
        }
        return V();
    }

    V second_value() const {
        for(auto it = vec->begin(); it; ++it) {
            if(it->third) {
                for(++it; it; ++it) {
                    if(it->third) {
                        return it->second;
                    }
                }
            }
        }
        return V();
    }

    K last_value() const {
        for(long long i = vec->size()-1; i >= 0; --i) {
            auto entry = vec->nth(i);
            if(entry.third) {
                return entry.second;
            }
        }
        return K();
    }

    // TODO: this is not fast... speedup by caching first value index.
    hash_map_ptr_t rest() const {
        hash_map_ptr_t copy = new_hash_map(*this);
        for(size_t i = 0; i < vec->size(); ++i) {
            auto entry = vec->nth(i);
            if(entry.third) {
                copy->vec = copy->vec->assoc(i, entry_t(entry.first, V(), false));
                --copy->length;
                break;
            }
        }
        return copy;
    }

    hash_map_ptr_t drop(size_t n) const {
        hash_map_ptr_t copy = new_hash_map(*this);
        for(size_t i = 0; i < vec->size() && n; ++i) {
            auto entry = vec->nth(i);
            if(entry.third) {
                copy->vec = copy->vec->assoc(i, entry_t(entry.first, V(), false));
                --copy->length;
                --n;
            }
        }
        return copy;
    }

    hash_map_ptr_t take(size_t n) const {
        hash_map_ptr_t copy = new_hash_map();
        for(size_t i = 0; i < vec->size() && copy->length < n; ++i) {
            auto entry = vec->nth(i);
            if(entry.third) {
                copy->assoc_inplace(entry.first, entry.second);
            }
        }
        return copy;
    }

private:

    hash_map_t *assoc_inplace(const K &key, const V &value) {
        if(vec->size() - length < vec->size() / 8) {
            resize_inplace(vec->size() * 2);
        }
        int index = jo_hash_value(key) % vec->size();
        // TODO: nth twice? optimization pls
        while(vec->nth(index).third) {
            if(vec->nth(index).first == key) {
                vec->assoc_inplace(index, entry_t(key, value, true));
                return this;
            }
            index = (index + 1) % vec->size();
        } 
        vec->assoc_inplace(index, entry_t(key, value, true));
        ++length;
        return this;
    }

};

hash_map_t::alloc_t hash_map_t::alloc;

struct jo_persistent_hash_set;
typedef jo_persistent_hash_set hash_set_t;
typedef jo_alloc_t<hash_set_t> hash_set_alloc_t;
hash_set_alloc_t hash_set_alloc;
typedef jo_shared_ptr_t<hash_set_t> hash_set_ptr_t;

template<typename... A>
static hash_set_ptr_t new_hash_set(A... args) { return hash_set_ptr_t(hash_set_alloc.emplace(args...)); }

typedef jo_pair<node_idx_t, bool> jo_persistent_hash_set_entry_t;
typedef jo_persistent_vector_node_t<jo_persistent_hash_set_entry_t> hash_set_node_t;
typedef jo_alloc_t<jo_persistent_hash_set_entry_t> hash_set_node_alloc_t;
hash_set_node_alloc_t hash_set_node_alloc;
typedef jo_shared_ptr_t<hash_set_node_t> hash_set_node_ptr_t;

template<> 
jo_persistent_vector<jo_persistent_hash_set_entry_t>::vector_node_alloc_t 
    jo_persistent_vector<jo_persistent_hash_set_entry_t>::alloc_node 
    = jo_persistent_vector<jo_persistent_hash_set_entry_t>::vector_node_alloc_t();
template<> 
jo_persistent_vector<jo_persistent_hash_set_entry_t>::vector_alloc_t 
    jo_persistent_vector<jo_persistent_hash_set_entry_t>::alloc
    = jo_persistent_vector<jo_persistent_hash_set_entry_t>::vector_alloc_t();

// jo_persistent_hash_set
class jo_persistent_hash_set : jo_object {
    typedef node_idx_t K;
    // the way this works is that you make your hash value of K and
    // then use that as an index into the vector.
    typedef jo_pair<K, bool> entry_t;

    typedef jo_persistent_vector<entry_t> vector_t;
    typedef jo_alloc_t<vector_t> alloc_t;
    typedef jo_shared_ptr_t<vector_t> ptr_t;

    static alloc_t alloc;

    template<typename...A> ptr_t new_vec(A... args) const { return ptr_t(alloc.emplace(args...)); }

    // vec is used to store the keys and values
    ptr_t vec;

    // number of entries actually used in the hash table
    size_t length;

public:
    jo_persistent_hash_set() : vec(new_vec(32)), length() {}
    jo_persistent_hash_set(const jo_persistent_hash_set &other) : vec(other.vec), length(other.length) {}
    jo_persistent_hash_set &operator=(const jo_persistent_hash_set &other) {
        vec = other.vec;
        length = other.length;
        return *this;
    }

    size_t size() const { return length; }
    bool empty() const { return !length; }

    // iterator
    class iterator {
        typename jo_persistent_vector< entry_t >::iterator cur;
    public:
        iterator(const typename jo_persistent_vector< entry_t >::iterator &it) : cur(it) {
            // find first non-erased entry
            while(cur && !cur->second) {
                cur++;
            }
        }
        iterator() : cur() {}
        iterator &operator++() {
            if(cur) do {
                cur++;
            } while(cur && !cur->second);
            return *this;
        }
        iterator operator++(int) {
            iterator ret = *this;
            if(cur) do {
                cur++;
            } while(cur && !cur->second);
            return ret;
        }
        bool operator==(const iterator &other) const { return cur == other.cur; }
        bool operator!=(const iterator &other) const { return cur != other.cur; }
        const entry_t &operator*() const { return *cur; }
        const entry_t *operator->() const { return &*cur; }
        operator bool() const { return cur; }
        iterator operator+(int i) const {
            iterator ret = *this;
            ret.cur += i;
            return ret;
        }
    };

    iterator begin() { return iterator(vec->begin()); }
    iterator begin() const { return iterator(vec->begin()); }
    iterator end() { return iterator(vec->end()); }
    iterator end() const { return iterator(vec->end()); }

    hash_set_ptr_t resize(size_t new_size) const {
        hash_set_ptr_t copy = new_hash_set();
        copy->vec = new_vec(new_size);
        for(iterator it = begin(); it; ++it) {
            int index = jo_hash_value(it->first) % new_size;
            while(copy->vec->nth(index).second) {
                index = (index + 1) % new_size;
            }
            copy->vec->assoc_inplace(index, entry_t(it->first, true));
        }
        return copy;
    }

    auto resize_inplace(size_t new_size) {
        ptr_t nv = new_vec(new_size);
        for(iterator it = begin(); it; ++it) {
            int index = jo_hash_value(it->first) % new_size;
            while(nv->nth(index).second) {
                index = (index + 1) % new_size;
            }
            nv->assoc_inplace(index, entry_t(it->first, true));
        }
        vec = nv;
        return this;
    }

    // assoc with lambda for equality
    template<typename F>
    hash_set_ptr_t assoc(const K &key, F eq) const {
        hash_set_ptr_t copy = new_hash_set(*this);
        if(copy->vec->size() - copy->length < copy->vec->size() / 8) {
            copy->resize_inplace(copy->vec->size() * 2);
        }
        int index = jo_hash_value(key) % copy->vec->size();
        entry_t e = copy->vec->nth(index);
        while(e.second) {
            if(eq(e.first, key)) {
                copy->vec = copy->vec->assoc(index, entry_t(key, true));
                return copy;
            }
            index = (index + 1) % copy->vec->size();
            e = copy->vec->nth(index);
        } 
        copy->vec = copy->vec->assoc(index, entry_t(key, true));
        ++copy->length;
        return copy;
    }

    template<typename F>
    auto assoc_inplace(const K &key, F eq) {
        if(vec->size() - length < vec->size() / 8) {
            resize_inplace(vec->size() * 2);
        }
        int index = jo_hash_value(key) % vec->size();
        entry_t e = vec->nth(index);
        while(e.second) {
            if(eq(e.first, key)) {
                vec->assoc_inplace(index, entry_t(key, true));
                return this;
            }
            index = (index + 1) % vec->size();
            e = vec->nth(index);
        } 
        vec->assoc_inplace(index, entry_t(key, true));
        ++length;
        return this;
    }

    // dissoc with lambda for equality
    template<typename F>
    hash_set_ptr_t dissoc(const K &key, F eq) const {
        hash_set_ptr_t copy = new_hash_set(*this);
        int index = jo_hash_value(key) % copy->vec->size();
        entry_t e = copy->vec->nth(index);
        while(e.second) {
            if(eq(e.first, key)) {
                copy->vec = copy->vec->assoc(index, entry_t());
                --copy->length;
                // need to shuffle entries up to fill in the gap
                int i = index;
                int j = i;
                while(true) {
                    j = (j + 1) % copy->vec->size();
                    if(!copy->vec->nth(j).second) {
                        break;
                    }
                    entry_t next_entry = copy->vec->nth(j);
                    if((jo_hash_value(next_entry.first) % copy->vec->size()) <= i) {
                        copy->vec = copy->vec->assoc(i, next_entry);
                        copy->vec = copy->vec->assoc(j, entry_t());
                        i = j;
                    }
                }
                return copy;
            }
            index = (index + 1) % copy->vec->size();
            e = copy->vec->nth(index);
        } 
        return copy;
    }

    // find using lambda
    template<typename F>
    entry_t find(const K &key, const F &f) {
        if(vec->size() == 0) {
            return entry_t();
        }
        size_t index = jo_hash_value(key) % vec->size();
        entry_t e = vec->nth(index);
        while(e.second) {
            if(f(e.first, key)) {
                return vec->nth(index);
            }
            index = (index + 1) % vec->size();
            e = vec->nth(index);
        } 
        return entry_t();
    }
     
    // contains using lambda
    template<typename F>
    bool contains(const K &key, const F &f) {
        return find(key, f).second;
    }

    // conj
    template<typename F>
    hash_set_ptr_t conj(hash_set_ptr_t other, const F &f) const {
        hash_set_ptr_t copy = new_hash_set(*this);
        for(auto it = other->vec->begin(); it; ++it) {
            if(it->second) {
                copy = copy->assoc(it->first, f);
            }
        }
        return copy;
    }

    K first_value() const {
        for(auto it = vec->begin(); it; ++it) {
            if(it->second) {
                return it->first;
            }
        }
        return K();
    }

    K second_value() const {
        for(auto it = vec->begin(); it; ++it) {
            if(it->second) {
                for(++it; it; ++it) {
                    if(it->second) {
                        return it->first;
                    }
                }
            }
        }
        return K();
    }

    K last_value() const {
        for(long long i = vec->size()-1; i >= 0; --i) {
            auto entry = vec->nth(i);
            if(entry.second) {
                return entry.first;
            }
        }
        return K();
    }

    // TODO: this is not fast... speedup by caching first key index.
    hash_set_ptr_t rest() const {
        hash_set_ptr_t copy = new_hash_set(*this);
        for(size_t i = 0; i < vec->size(); ++i) {
            auto entry = vec->nth(i);
            if(entry.second) {
                copy->vec = copy->vec->assoc(i, entry_t(entry.first, false));
                --copy->length;
                break;
            }
        }
        return copy;
    }

    hash_set_ptr_t drop(size_t n) const {
        hash_set_ptr_t copy = new_hash_set(*this);
        for(size_t i = 0; i < vec->size() && n; ++i) {
            auto entry = vec->nth(i);
            if(entry.second) {
                copy->vec = copy->vec->assoc(i, entry_t(entry.first, false));
                --copy->length;
                --n;
            }
        }
        return copy;
    }

    hash_set_ptr_t take(size_t n) const {
        hash_set_ptr_t copy = new_hash_set();
        for(size_t i = 0; i < vec->size() && copy->length < n; ++i) {
            auto entry = vec->nth(i);
            if(entry.second) {
                copy->assoc_inplace(entry.first);
            }
        }
        return copy;
    }

private:

    hash_set_t *assoc_inplace(const K &key) {
        if(vec->size() - length < vec->size() / 8) {
            resize_inplace(vec->size() * 2);
        }
        int index = jo_hash_value(key) % vec->size();
        entry_t e = vec->nth(index);
        while(e.second) {
            if(e.first == key) {
                vec->assoc_inplace(index, entry_t(key, true));
                return this;
            }
            index = (index + 1) % vec->size();
            e = vec->nth(index);
        } 
        vec->assoc_inplace(index, entry_t(key, true));
        ++length;
        return this;
    }
};

hash_set_t::alloc_t hash_set_t::alloc;

struct jo_persistent_queue;
typedef jo_persistent_queue queue_t;
typedef jo_alloc_t<queue_t> queue_alloc_t;
queue_alloc_t queue_alloc;
typedef jo_shared_ptr_t<queue_t> queue_ptr_t;

template<typename... A>
static queue_ptr_t new_queue(A... args) { return queue_ptr_t(queue_alloc.emplace(args...)); }

struct jo_persistent_queue : jo_object {
    typedef queue_ptr_t shared_ptr;

    hash_map_ptr_t map;
    size_t read_index;
    size_t write_index;

    jo_persistent_queue() : map(new_hash_map()), read_index(0), write_index(0) {}
    jo_persistent_queue(const jo_persistent_queue &other) : map(other.map), read_index(other.read_index), write_index(other.write_index) {}
    jo_persistent_queue(jo_persistent_queue &&other) : map(other.map), read_index(other.read_index), write_index(other.write_index) {
        other.map = nullptr;
        other.read_index = 0;
        other.write_index = 0;
    }

    jo_persistent_queue &operator=(const jo_persistent_queue &other) {
        map = other.map;
        read_index = other.read_index;
        write_index = other.write_index;
        return *this;
    }

    jo_persistent_queue &operator=(jo_persistent_queue &&other) {
        map = other.map;
        read_index = other.read_index;
        write_index = other.write_index;
        other.map = nullptr;
        other.read_index = 0;
        other.write_index = 0;
        return *this;
    }

    shared_ptr clone() const {
        return new_queue(*this);
    }

    shared_ptr push(const node_idx_t &key) const {
        shared_ptr copy = clone();
        size_t index = copy->write_index++;
        copy->map = copy->map->assoc(new_node_int(index), key, node_eq);
        return copy;
    }

    void push_inplace(const node_idx_t &key) {
        size_t index = write_index++;
        map->assoc_inplace(new_node_int(index), key, node_eq);
    }

    jo_pair<shared_ptr, node_idx_t> pop() const {
        shared_ptr copy = clone();
        if(read_index >= write_index) {
            return jo_make_pair<shared_ptr, node_idx_t>(copy, 0);
        }
        size_t index = copy->read_index++;
        node_idx_t key = new_node_int(index);
        auto entry = copy->map->find(key, node_eq);
        if(entry.third) {
            copy->map = copy->map->dissoc(key, node_eq);
            return jo_make_pair<shared_ptr, node_idx_t>(copy, entry.second);
        }
        return jo_make_pair<shared_ptr, node_idx_t>(copy, 0);
    }

    node_idx_t pop_inplace() {
        if(read_index >= write_index) {
            return 0;
        }
        size_t index = read_index++;
        node_idx_t key = new_node_int(index);
        auto entry = map->find(key, node_eq);
        if(entry.third) {
            map->dissoc_inplace(key, node_eq);
            return entry.second;
        }
        return 0;
    }

    node_idx_t peek() const {
        if(read_index >= write_index) {
            return 0;
        }
        size_t index = read_index;
        node_idx_t key = new_node_int(index);
        auto entry = map->find(key, node_eq);
        if(entry.third) {
            return entry.second;
        }
        return 0;
    }
};