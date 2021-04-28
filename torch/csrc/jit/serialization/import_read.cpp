#include <torch/csrc/jit/serialization/import_read.h>

namespace torch {
namespace jit {

IValue readArchiveAndTensors(
    const std::string& archive_name,
    c10::optional<TypeResolver> type_resolver,
    c10::optional<ObjLoader> obj_loader,
    c10::optional<at::Device> device,
    caffe2::serialize::PyTorchStreamReader& stream_reader) {
  std::string picklename = archive_name + ".pkl";
  at::DataPtr pickle_ptr;
  size_t pickle_size = 0;
  std::tie(pickle_ptr, pickle_size) = stream_reader.getRecord(picklename);

  size_t bytes_read = 0;
  auto data = reinterpret_cast<const char*>(pickle_ptr.get());
  auto reader = [&](char* buffer, size_t len) -> size_t {
    if (bytes_read >= pickle_size) {
      return 0;
    }
    len = std::min(pickle_size - bytes_read, len);
    // Copy len bytes into buffer
    const char* start = data + bytes_read;
    std::memcpy(buffer, start, len);
    bytes_read += len;
    return len;
  };

  std::string archive_name_plus_slash = archive_name + "/";
  auto read_record = [&](const std::string& name) {
    std::string ss = archive_name_plus_slash + name;
    return std::get<0>(stream_reader.getRecord(ss));
  };

  Unpickler unpickler(
      reader,
      type_resolver ? std::move(*type_resolver) : nullptr,
      obj_loader ? std::move(*obj_loader) : nullptr,
      std::move(read_record),
      device);
  unpickler.set_version(stream_reader.version());
  return unpickler.parse_ivalue();
}

} // namespace jit
} // namespace torch
