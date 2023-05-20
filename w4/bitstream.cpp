#include "bitstream.hpp"

#include <cassert>

Bitstream::Bitstream(const void* data, size_t size) {
  data_ = new uint8_t[size];
  capacity_ = size;
  write_offset_ = capacity_;

  std::memcpy(data_, data, size);
}

Bitstream::~Bitstream() {
  if (data_ != nullptr) {
    delete[] data_;
  }

  data_ = nullptr;
  capacity_ = 0;
  read_offset_ = 0;
  write_offset_ = 0;
}

size_t Bitstream::Size() const {
  assert(write_offset_ >= read_offset_);
  return write_offset_ - read_offset_;
}

void Bitstream::Write(const void* data, size_t size) {
  assert(data);

  if (write_offset_ + size > capacity_) {
    uint8_t* new_data = new uint8_t[write_offset_ + size];

    if (data_ != nullptr) {
      std::memcpy(new_data, data_, write_offset_);
    }

    data_ = new_data;
    capacity_ = write_offset_ + size;
  }

  std::memcpy(data_ + write_offset_, data, size);
  write_offset_ += size;
}

void Bitstream::Read(void* out_data, size_t size) {
  assert(out_data);
  assert(read_offset_ + size <= write_offset_);

  std::memcpy(out_data, data_ + read_offset_, size);
  read_offset_ += size;
}

void Bitstream::Skip(size_t size) {
  assert(read_offset_ + size <= write_offset_);
  read_offset_ += size;
}