#include "app.h"
#include "zlib.h"

// Memory-mapped file opening
MemoryMap* mmap_open(const wchar_t* path) {
    if (!path) return nullptr;
    auto* m = new MemoryMap{};
    // Zero-initialization would set file_handle to nullptr, but mmap_close checks
    // for INVALID_HANDLE_VALUE. Initialize explicitly so partially-failed mmap state
    // is safe to pass to mmap_close.
    m->file_handle = INVALID_HANDLE_VALUE;
    m->mapping_handle = nullptr;
    m->data = nullptr;
    m->size = 0;
    m->file_handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m->file_handle == INVALID_HANDLE_VALUE) {
        delete m;
        return nullptr;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(m->file_handle, &file_size)) {
        CloseHandle(m->file_handle);
        delete m;
        return nullptr;
    }
    m->size = static_cast<size_t>(file_size.QuadPart);

    if (m->size == 0) {
        // Empty file - valid but no mapping needed
        m->mapping_handle = nullptr;
        m->data = nullptr;
        return m;
    }

    m->mapping_handle = CreateFileMappingW(m->file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!m->mapping_handle) {
        CloseHandle(m->file_handle);
        delete m;
        return nullptr;
    }

    m->data = MapViewOfFile(m->mapping_handle, FILE_MAP_READ, 0, 0, 0);
    if (!m->data) {
        CloseHandle(m->mapping_handle);
        CloseHandle(m->file_handle);
        delete m;
        return nullptr;
    }

    return m;
}

// Close memory-mapped file
void mmap_close(MemoryMap* m) {
    if (!m) return;
    if (m->data) UnmapViewOfFile(m->data);
    if (m->mapping_handle) CloseHandle(m->mapping_handle);
    if (m->file_handle != INVALID_HANDLE_VALUE) CloseHandle(m->file_handle);
    delete m;
}

// Check if file exists
bool file_exists(const wchar_t* path) {
    if (!path) return false;
    DWORD attrs = GetFileAttributesW(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

// Check if directory exists
bool dir_exists(const wchar_t* path) {
    if (!path) return false;
    DWORD attrs = GetFileAttributesW(path);
    return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

// Recursively delete a directory and all its contents
bool delete_directory_recursive(const wchar_t* path) {
    if (!path || !dir_exists(path)) return false;

    // SHFileOperation requires double-null terminated string
    wchar_t path_buf[MAX_PATH + 2] = {};
    wcsncpy_s(path_buf, MAX_PATH, path, _TRUNCATE);
    path_buf[wcslen(path_buf) + 1] = L'\0';  // Double null terminate

    SHFILEOPSTRUCTW op = {};
    op.wFunc = FO_DELETE;
    op.pFrom = path_buf;
    op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    return SHFileOperationW(&op) == 0;
}

// Read entire file into buffer (for small files like refs)
// Returns bytes read, 0 on failure
size_t read_file(const wchar_t* path, char* buffer, size_t buffer_size) {
    if (!path || !buffer || buffer_size == 0) return 0;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 0;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file, &file_size)) {
        CloseHandle(file);
        return 0;
    }

    size_t to_read = static_cast<size_t>(file_size.QuadPart);
    if (to_read >= buffer_size) {
        to_read = buffer_size - 1;  // Leave room for null terminator
    }

    DWORD bytes_read = 0;
    BOOL success = ReadFile(file, buffer, static_cast<DWORD>(to_read), &bytes_read, nullptr);
    CloseHandle(file);

    if (!success) {
        return 0;
    }

    buffer[bytes_read] = '\0';
    return bytes_read;
}

// Decompress zlib-compressed data (for git loose objects)
// Returns decompressed size, 0 on failure
// Output buffer should be large enough for decompressed data
size_t zlib_decompress(const void* compressed, size_t compressed_size,
                       void* output, size_t output_size) {
    if (!compressed || !output || compressed_size == 0 || output_size == 0) return 0;
    // zlib avail_in/avail_out are uInt (32-bit on MSVC). Sizes that do not fit
    // would truncate (e.g. 4GiB → 0) and silently mis-inflate.
    if (compressed_size > static_cast<size_t>(UINT_MAX) ||
        output_size > static_cast<size_t>(UINT_MAX)) {
        return 0;
    }
    z_stream strm = {};
    strm.next_in = static_cast<Bytef*>(const_cast<void*>(compressed));
    strm.avail_in = static_cast<uInt>(compressed_size);
    strm.next_out = static_cast<Bytef*>(output);
    strm.avail_out = static_cast<uInt>(output_size);

    // Initialize for raw inflate (zlib format, not gzip)
    if (inflateInit(&strm) != Z_OK) {
        return 0;
    }

    int ret = inflate(&strm, Z_FINISH);
    size_t decompressed_size = strm.total_out;

    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        return 0;
    }

    return decompressed_size;
}

// Compress data using zlib (for writing git loose objects)
// Returns compressed size, 0 on failure
// Output buffer should be at least input_size + 64 bytes (worst case overhead)
size_t zlib_compress(const void* input, size_t input_size,
                     void* output, size_t output_size) {
    if (!input || !output || input_size == 0 || output_size == 0) return 0;
    if (input_size > static_cast<size_t>(UINT_MAX) ||
        output_size > static_cast<size_t>(UINT_MAX)) {
        return 0;
    }
    z_stream strm = {};
    strm.next_in = static_cast<Bytef*>(const_cast<void*>(input));
    strm.avail_in = static_cast<uInt>(input_size);
    strm.next_out = static_cast<Bytef*>(output);
    strm.avail_out = static_cast<uInt>(output_size);

    // Initialize deflate with default compression level
    // Z_DEFAULT_COMPRESSION = 6, good balance of speed/size
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
        return 0;
    }

    int ret = deflate(&strm, Z_FINISH);
    size_t compressed_size = strm.total_out;

    deflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        return 0;
    }

    return compressed_size;
}

