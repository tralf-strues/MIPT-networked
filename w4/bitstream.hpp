#pragma once

#include <cstdint>
#include <cstring>

class Bitstream {
public:
  Bitstream() = default;
  Bitstream(const void* data, size_t size);

  ~Bitstream();

  Bitstream(const Bitstream& other) = delete;

  size_t Size() const;

  void Write(const void* data, size_t size);
  void Read(void* out_data, size_t size);
  void Skip(size_t size);

  template<typename T>
  void Write(const T& value) {
    Write(&value, sizeof(T));
  }

  template<typename T>
  void Read(T& out_value) {
    Read(&out_value, sizeof(T));
  }

  template<typename T>
  void Skip() {
    Skip(sizeof(T));
  }

private:
  uint8_t* data_{nullptr};
  size_t capacity_{0};

  size_t read_offset_{0};
  size_t write_offset_{0};
};