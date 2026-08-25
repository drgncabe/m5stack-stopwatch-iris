#pragma once

#include <Arduino.h>
#include <FS.h>
#include <Preferences.h>

namespace iris {

enum class BadgeDisplayMode : uint8_t {
  Fit = 0,
  Fill = 1,
  Center = 2,
};

enum class BadgeAssetType : uint8_t {
  None = 0,
  Png = 1,
  Jpeg = 2,
  Gif = 3,
};

struct BadgeMetadata {
  String filename;
  String path;
  String contentType;
  BadgeAssetType type = BadgeAssetType::None;
  BadgeDisplayMode mode = BadgeDisplayMode::Fit;
  uint32_t sizeBytes = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  bool keepAwake = false;
  bool valid = false;
};

class BadgeService {
 public:
  bool begin();
  bool mounted() const { return mounted_; }
  const BadgeMetadata& metadata() const { return metadata_; }

  BadgeDisplayMode mode() const { return metadata_.mode; }
  void setMode(BadgeDisplayMode mode);
  void nextMode();
  bool keepAwake() const { return metadata_.keepAwake; }
  void setKeepAwake(bool enabled);

  bool hasAsset() const;
  bool isStaticRenderable() const;
  bool beginUpload(const String& filename, const String& contentType);
  bool writeUpload(const uint8_t* data, size_t size);
  bool finishUpload();
  void abortUpload();
  bool deleteBadge();
  String statusText() const;
  String typeName() const;
  String modeName() const;
  String json() const;
  uint32_t storageTotalBytes() const;
  uint32_t storageUsedBytes() const;
  uint32_t storageFreeBytes() const;

  static const char* modeName(BadgeDisplayMode mode);

 private:
  static constexpr uint32_t kMaxBadgeBytes = 4UL * 1024UL * 1024UL;
  static constexpr const char* kBadgePath = "/badge.bin";
  static constexpr const char* kBadgeTempPath = "/badge.tmp";

  bool loadMetadata();
  void saveMetadata();
  void clearMetadata();
  bool extensionAllowed(const String& filename) const;
  bool sniffAsset(BadgeAssetType* type, uint16_t* width, uint16_t* height, String* contentType);
  bool sniffPng(const uint8_t* header, size_t size, uint16_t* width, uint16_t* height) const;
  bool sniffGif(const uint8_t* header, size_t size, uint16_t* width, uint16_t* height) const;
  bool sniffJpeg(uint16_t* width, uint16_t* height) const;
  String sanitizedFilename(const String& filename) const;
  const char* contentTypeFor(BadgeAssetType type) const;
  const char* typeName(BadgeAssetType type) const;

  Preferences prefs_;
  BadgeMetadata metadata_;
  fs::File uploadFile_;
  String uploadFilename_;
  String uploadContentType_;
  uint32_t uploadSize_ = 0;
  bool mounted_ = false;
  bool uploadRejected_ = false;
};

}  // namespace iris
