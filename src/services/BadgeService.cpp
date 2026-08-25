#include "iris/services/BadgeService.h"

#include <SPIFFS.h>
#include <ctype.h>
#include <string.h>

namespace iris {

namespace {
uint16_t readBe16(const uint8_t* data) {
  return static_cast<uint16_t>((data[0] << 8) | data[1]);
}

uint32_t readBe32(const uint8_t* data) {
  return (static_cast<uint32_t>(data[0]) << 24) |
         (static_cast<uint32_t>(data[1]) << 16) |
         (static_cast<uint32_t>(data[2]) << 8) |
         static_cast<uint32_t>(data[3]);
}

uint16_t readLe16(const uint8_t* data) {
  return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

String escapeJsonValue(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '\\' || c == '"') {
      escaped += '\\';
      escaped += c;
    } else {
      escaped += c;
    }
  }
  return escaped;
}
}  // namespace

bool BadgeService::begin() {
  prefs_.begin("iris_badge", false);
  mounted_ = SPIFFS.begin(true);
  loadMetadata();
  if (!mounted_ || !SPIFFS.exists(kBadgePath)) {
    clearMetadata();
  }
  return mounted_;
}

void BadgeService::setMode(BadgeDisplayMode mode) {
  metadata_.mode = mode;
  saveMetadata();
}

void BadgeService::nextMode() {
  const uint8_t next = (static_cast<uint8_t>(metadata_.mode) + 1) %
                       (static_cast<uint8_t>(BadgeDisplayMode::Center) + 1);
  setMode(static_cast<BadgeDisplayMode>(next));
}

void BadgeService::setKeepAwake(bool enabled) {
  metadata_.keepAwake = enabled;
  saveMetadata();
}

bool BadgeService::hasAsset() const {
  return mounted_ && metadata_.valid && SPIFFS.exists(kBadgePath);
}

bool BadgeService::isStaticRenderable() const {
  return hasAsset() &&
         (metadata_.type == BadgeAssetType::Png || metadata_.type == BadgeAssetType::Jpeg);
}

bool BadgeService::beginUpload(const String& filename, const String& contentType) {
  abortUpload();
  uploadRejected_ = false;
  if (!mounted_ || !extensionAllowed(filename)) {
    uploadRejected_ = true;
    return false;
  }

  uploadFilename_ = sanitizedFilename(filename);
  uploadContentType_ = contentType;
  uploadSize_ = 0;
  SPIFFS.remove(kBadgeTempPath);
  uploadFile_ = SPIFFS.open(kBadgeTempPath, FILE_WRITE);
  if (!uploadFile_) {
    uploadRejected_ = true;
    return false;
  }
  return true;
}

bool BadgeService::writeUpload(const uint8_t* data, size_t size) {
  if (uploadRejected_ || !uploadFile_) return false;
  if (uploadSize_ + size > kMaxBadgeBytes) {
    abortUpload();
    uploadRejected_ = true;
    return false;
  }
  if (uploadFile_.write(data, size) != size) {
    abortUpload();
    uploadRejected_ = true;
    return false;
  }
  uploadSize_ += size;
  return true;
}

bool BadgeService::finishUpload() {
  if (uploadRejected_ || !uploadFile_) {
    abortUpload();
    return false;
  }

  uploadFile_.close();
  BadgeAssetType type = BadgeAssetType::None;
  uint16_t width = 0;
  uint16_t height = 0;
  String contentType;
  if (!sniffAsset(&type, &width, &height, &contentType)) {
    abortUpload();
    return false;
  }

  SPIFFS.remove(kBadgePath);
  if (!SPIFFS.rename(kBadgeTempPath, kBadgePath)) {
    abortUpload();
    return false;
  }

  metadata_.filename = uploadFilename_;
  metadata_.path = kBadgePath;
  metadata_.contentType = contentType;
  metadata_.type = type;
  metadata_.sizeBytes = uploadSize_;
  metadata_.width = width;
  metadata_.height = height;
  metadata_.valid = true;
  saveMetadata();
  uploadFilename_ = "";
  uploadContentType_ = "";
  uploadSize_ = 0;
  uploadRejected_ = false;
  return true;
}

void BadgeService::abortUpload() {
  if (uploadFile_) uploadFile_.close();
  if (mounted_) SPIFFS.remove(kBadgeTempPath);
  uploadFilename_ = "";
  uploadContentType_ = "";
  uploadSize_ = 0;
}

bool BadgeService::deleteBadge() {
  abortUpload();
  bool removed = true;
  if (mounted_ && SPIFFS.exists(kBadgePath)) {
    removed = SPIFFS.remove(kBadgePath);
  }
  clearMetadata();
  return removed;
}

String BadgeService::statusText() const {
  if (!mounted_) return "Storage unavailable";
  if (!hasAsset()) return "Default Iris badge";
  return metadata_.filename;
}

String BadgeService::typeName() const {
  return typeName(metadata_.type);
}

String BadgeService::modeName() const {
  return modeName(metadata_.mode);
}

String BadgeService::json() const {
  String json;
  json.reserve(420);
  json += F("{\"mounted\":");
  json += mounted_ ? F("true") : F("false");
  json += F(",\"hasAsset\":");
  json += hasAsset() ? F("true") : F("false");
  json += F(",\"filename\":\"");
  json += escapeJsonValue(metadata_.filename);
  json += F("\",\"contentType\":\"");
  json += escapeJsonValue(metadata_.contentType);
  json += F("\",\"type\":\"");
  json += typeName();
  json += F("\",\"mode\":\"");
  json += modeName();
  json += F("\",\"sizeBytes\":");
  json += String(metadata_.sizeBytes);
  json += F(",\"width\":");
  json += String(metadata_.width);
  json += F(",\"height\":");
  json += String(metadata_.height);
  json += F(",\"keepAwake\":");
  json += metadata_.keepAwake ? F("true") : F("false");
  json += F(",\"storageTotalBytes\":");
  json += String(storageTotalBytes());
  json += F(",\"storageUsedBytes\":");
  json += String(storageUsedBytes());
  json += F(",\"storageFreeBytes\":");
  json += String(storageFreeBytes());
  json += F(",\"gifPlayback\":\"");
  json += metadata_.type == BadgeAssetType::Gif ? F("stored; decoder pending") : F("not applicable");
  json += F("\"}");
  return json;
}

uint32_t BadgeService::storageTotalBytes() const {
  return mounted_ ? SPIFFS.totalBytes() : 0;
}

uint32_t BadgeService::storageUsedBytes() const {
  return mounted_ ? SPIFFS.usedBytes() : 0;
}

uint32_t BadgeService::storageFreeBytes() const {
  const uint32_t total = storageTotalBytes();
  const uint32_t used = storageUsedBytes();
  return total > used ? total - used : 0;
}

const char* BadgeService::modeName(BadgeDisplayMode mode) {
  switch (mode) {
    case BadgeDisplayMode::Fill: return "Fill";
    case BadgeDisplayMode::Center: return "Center";
    default: return "Fit";
  }
}

bool BadgeService::loadMetadata() {
  metadata_.filename = prefs_.getString("name", "");
  metadata_.path = kBadgePath;
  metadata_.contentType = prefs_.getString("type", "");
  metadata_.type = static_cast<BadgeAssetType>(prefs_.getUChar("asset", 0));
  metadata_.mode = static_cast<BadgeDisplayMode>(prefs_.getUChar("mode", 0));
  if (static_cast<uint8_t>(metadata_.mode) > static_cast<uint8_t>(BadgeDisplayMode::Center)) {
    metadata_.mode = BadgeDisplayMode::Fit;
  }
  metadata_.sizeBytes = prefs_.getULong("bytes", 0);
  metadata_.width = prefs_.getUShort("width", 0);
  metadata_.height = prefs_.getUShort("height", 0);
  metadata_.keepAwake = prefs_.getBool("awake", false);
  metadata_.valid = prefs_.getBool("valid", false) &&
                    metadata_.type != BadgeAssetType::None &&
                    metadata_.sizeBytes > 0;
  return metadata_.valid;
}

void BadgeService::saveMetadata() {
  prefs_.putString("name", metadata_.filename);
  prefs_.putString("type", metadata_.contentType);
  prefs_.putUChar("asset", static_cast<uint8_t>(metadata_.type));
  prefs_.putUChar("mode", static_cast<uint8_t>(metadata_.mode));
  prefs_.putULong("bytes", metadata_.sizeBytes);
  prefs_.putUShort("width", metadata_.width);
  prefs_.putUShort("height", metadata_.height);
  prefs_.putBool("awake", metadata_.keepAwake);
  prefs_.putBool("valid", metadata_.valid);
}

void BadgeService::clearMetadata() {
  const BadgeDisplayMode mode = metadata_.mode;
  const bool keepAwake = metadata_.keepAwake;
  metadata_ = BadgeMetadata{};
  metadata_.path = kBadgePath;
  metadata_.mode = mode;
  metadata_.keepAwake = keepAwake;
  saveMetadata();
}

bool BadgeService::extensionAllowed(const String& filename) const {
  String lower = filename;
  lower.toLowerCase();
  return lower.endsWith(".png") || lower.endsWith(".jpg") ||
         lower.endsWith(".jpeg") || lower.endsWith(".gif");
}

bool BadgeService::sniffAsset(BadgeAssetType* type, uint16_t* width, uint16_t* height,
                              String* contentType) {
  if (!type || !width || !height || !contentType) return false;
  fs::File file = SPIFFS.open(kBadgeTempPath, FILE_READ);
  if (!file) return false;
  uint8_t header[32]{};
  const size_t count = file.read(header, sizeof(header));
  file.close();

  if (sniffPng(header, count, width, height)) {
    *type = BadgeAssetType::Png;
    *contentType = "image/png";
    return true;
  }
  if (sniffGif(header, count, width, height)) {
    *type = BadgeAssetType::Gif;
    *contentType = "image/gif";
    return true;
  }
  if (sniffJpeg(width, height)) {
    *type = BadgeAssetType::Jpeg;
    *contentType = "image/jpeg";
    return true;
  }
  return false;
}

bool BadgeService::sniffPng(const uint8_t* header, size_t size, uint16_t* width,
                            uint16_t* height) const {
  static constexpr uint8_t kPngSig[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  if (size < 24 || memcmp(header, kPngSig, sizeof(kPngSig)) != 0) return false;
  const uint32_t w = readBe32(header + 16);
  const uint32_t h = readBe32(header + 20);
  if (w == 0 || h == 0 || w > UINT16_MAX || h > UINT16_MAX) return false;
  *width = static_cast<uint16_t>(w);
  *height = static_cast<uint16_t>(h);
  return true;
}

bool BadgeService::sniffGif(const uint8_t* header, size_t size, uint16_t* width,
                            uint16_t* height) const {
  if (size < 10) return false;
  if (memcmp(header, "GIF87a", 6) != 0 && memcmp(header, "GIF89a", 6) != 0) return false;
  *width = readLe16(header + 6);
  *height = readLe16(header + 8);
  return *width > 0 && *height > 0;
}

bool BadgeService::sniffJpeg(uint16_t* width, uint16_t* height) const {
  fs::File file = SPIFFS.open(kBadgeTempPath, FILE_READ);
  if (!file) return false;
  if (file.read() != 0xFF || file.read() != 0xD8) {
    file.close();
    return false;
  }

  while (file.available()) {
    int markerPrefix = file.read();
    if (markerPrefix != 0xFF) continue;
    int marker = file.read();
    while (marker == 0xFF && file.available()) marker = file.read();
    if (marker < 0) break;
    if (marker == 0xD9 || marker == 0xDA) break;
    uint8_t lenBytes[2];
    if (file.read(lenBytes, 2) != 2) break;
    const uint16_t length = readBe16(lenBytes);
    if (length < 2) break;
    const bool sof = (marker >= 0xC0 && marker <= 0xC3) ||
                     (marker >= 0xC5 && marker <= 0xC7) ||
                     (marker >= 0xC9 && marker <= 0xCB) ||
                     (marker >= 0xCD && marker <= 0xCF);
    if (sof && length >= 7) {
      file.read();
      uint8_t dims[4];
      if (file.read(dims, 4) == 4) {
        *height = readBe16(dims);
        *width = readBe16(dims + 2);
        file.close();
        return *width > 0 && *height > 0;
      }
      break;
    }
    file.seek(file.position() + length - 2);
  }
  file.close();
  return false;
}

String BadgeService::sanitizedFilename(const String& filename) const {
  String clean;
  clean.reserve(32);
  for (size_t i = 0; i < filename.length() && clean.length() < 48; ++i) {
    const char c = filename[i];
    if (isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-') {
      clean += c;
    }
  }
  return clean.isEmpty() ? String("badge") : clean;
}

const char* BadgeService::contentTypeFor(BadgeAssetType type) const {
  switch (type) {
    case BadgeAssetType::Png: return "image/png";
    case BadgeAssetType::Jpeg: return "image/jpeg";
    case BadgeAssetType::Gif: return "image/gif";
    default: return "application/octet-stream";
  }
}

const char* BadgeService::typeName(BadgeAssetType type) const {
  switch (type) {
    case BadgeAssetType::Png: return "PNG";
    case BadgeAssetType::Jpeg: return "JPEG";
    case BadgeAssetType::Gif: return "GIF";
    default: return "Default";
  }
}

}  // namespace iris
