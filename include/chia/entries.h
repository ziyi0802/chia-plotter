/*
 * entries.h
 *
 *  Created on: May 22, 2021
 *      Author: mad
 */

#ifndef INCLUDE_CHIA_ENTRIES_H_
#define INCLUDE_CHIA_ENTRIES_H_

#include <chia/chia.h>
#include <chia/util.hpp>

#include <array>
#include <cstdio>
#include <cstdint>
#include <cstring>


namespace phase1 {

struct entry_1 {
	uint64_t y;			// 38 bit
	uint32_t x;			// 32 bit
	
	static constexpr uint32_t pos = 0;		// dummy
	static constexpr uint16_t off = 0;		// dummy
	static constexpr size_t disk_size = 9;
	
	size_t read(const uint8_t* buf) {
		y = 0;
		memcpy(&y, buf, 5);
		memcpy(&x, buf + 5, 4);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 5);
		memcpy(buf + 5, &x, 4);
		return disk_size;
	}
};

struct entry_t {
	uint64_t y;			// 38 bit
	uint32_t pos;		// 32 bit
	uint16_t off;		// 10 bit
};

template<int N>
struct entry_tx : entry_t {
	std::array<uint8_t, N * 4> meta;
	
	static constexpr size_t disk_size = 10 + N * 4;
	
	size_t read(const uint8_t* buf) {
		memcpy(&y, buf, 5);
		y &= 0x3FFFFFFFFFull;
		off = 0;
		off |= buf[4] >> 6;
		off |= uint16_t(buf[5]) << 2;
		memcpy(&pos, buf + 6, 4);
		memcpy(meta.data(), buf + 10, sizeof(meta));
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &y, 5);
		buf[4] |= off << 6;
		buf[5] = off >> 2;
		memcpy(buf + 6, &pos, 4);
		memcpy(buf + 10, meta.data(), sizeof(meta));
		return disk_size;
	}
};

typedef entry_tx<2> entry_2;
typedef entry_tx<4> entry_3;
typedef entry_tx<4> entry_4;
typedef entry_tx<3> entry_5;
typedef entry_tx<2> entry_6;

struct entry_7 : entry_t {
	static constexpr size_t disk_size = 0;
	size_t read(const uint8_t* buf) { return 0; }
	size_t write(uint8_t* buf) const { return 0; }
};

struct tmp_entry_1 {
	uint32_t x;			// 32 bit
	
	static constexpr size_t disk_size = 4;
	
	void assign(const entry_1& entry) {
		x = entry.x;
	}
	size_t read(const uint8_t* buf) {
		memcpy(&x, buf, 4);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &x, 4);
		return disk_size;
	}
};

struct tmp_entry_t {
	uint32_t pos;		// 32 bit
	uint16_t off;		// 10 bit
	
	static constexpr size_t disk_size = 6;
	
	void assign(const entry_t& entry) {
		pos = entry.pos;
		off = entry.off;
	}
	size_t read(const uint8_t* buf) {
		memcpy(&pos, buf, 4);
		memcpy(&off, buf + 4, 2);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &pos, 4);
		memcpy(buf + 4, &off, 2);
		return disk_size;
	}
};

template<typename T>
struct get_y {
	uint64_t operator()(const T& entry) {
		return entry.y;
	}
};

template<typename T>
struct get_meta {
	void operator()(const T& entry, uint8_t* bytes, size_t* num_bytes) {
		*num_bytes = sizeof(entry.meta);
		memcpy(bytes, entry.meta.data(), sizeof(entry.meta));
	}
};

template<>
struct get_meta<entry_1> {
	void operator()(const entry_1& entry, uint8_t* bytes, size_t* num_bytes) {
		*num_bytes = sizeof(uint32_t);
		const uint32_t tmp = bswap_32(entry.x);
		memcpy(bytes, &tmp, sizeof(uint32_t));
	}
};

template<typename T>
struct set_meta {
	void operator()(T& entry, const uint8_t* bytes, const size_t num_bytes) {
		if(num_bytes != sizeof(entry.meta)) {
			throw std::logic_error("meta data size mismatch");
		}
		memcpy(entry.meta.data(), bytes, sizeof(entry.meta));
	}
};

template<>
struct set_meta<entry_7> {
	void operator()(entry_7& entry, const uint8_t* bytes, const size_t num_bytes) {
		// no meta data
	}
};

} // phase1


namespace phase2 {

struct entry_t {
	uint32_t key;
	uint32_t pos;
	uint16_t off;		// 10 bit
	
	static constexpr size_t disk_size = 10;
	
	size_t read(const uint8_t* buf) {
		memcpy(&key, buf, 4);
		memcpy(&pos, buf + 4, 4);
		memcpy(&off, buf + 8, 2);
		return disk_size;
	}
	size_t write(uint8_t* buf) const {
		memcpy(buf, &key, 4);
		memcpy(buf + 4, &pos, 4);
		memcpy(buf + 8, &off, 2);
		return disk_size;
	}
};

template<typename T>
struct get_pos {
	uint64_t operator()(const T& entry) {
		return entry.pos;
	}
};

} // phase2


template<typename T>
bool write_entry(FILE* file, const T& entry) {
	uint8_t buf[T::disk_size];
	entry.write(buf);
	return fwrite(buf, 1, T::disk_size, file) == T::disk_size;
}

template<typename T>
bool read_entry(FILE* file, T& entry) {
	uint8_t buf[T::disk_size];
	if(fread(buf, 1, T::disk_size, file) != T::disk_size) {
		return false;
	}
	entry.read(buf);
	return true;
}


#endif /* INCLUDE_CHIA_ENTRIES_H_ */
