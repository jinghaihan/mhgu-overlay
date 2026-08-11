#pragma once

// Included by source/ui/main.cpp inside its private UI namespace.

class DamageTextRenderer {
public:
  s32 measure(const char* value, const float font_size) {
    if (!ensure_font()) {
      return 0;
    }
    return text_advance(value, font_size);
  }

  bool draw(
    tsl::gfx::Renderer* renderer,
    const char* value,
    const s32 left,
    const s32 baseline_y,
    const float font_size,
    const std::uint8_t alpha
  ) {
    if (!ensure_font() || alpha == 0) {
      return false;
    }

    const auto scale = stbtt_ScaleForPixelHeight(&font_, font_size);
    s32 min_x{};
    s32 max_x{};
    s32 min_y{};
    s32 max_y{};
    if (!text_bounds(
          value, scale, min_x, max_x, min_y, max_y
        )) {
      return false;
    }

    constexpr s32 kOutlineRadius = 2;
    constexpr s32 kShadowOffsetX = 3;
    constexpr s32 kShadowOffsetY = 3;
    const auto canvas_min_x = min_x - kOutlineRadius;
    const auto canvas_min_y = min_y - kOutlineRadius;
    const auto canvas_max_x =
      max_x + std::max(kOutlineRadius, kShadowOffsetX);
    const auto canvas_max_y =
      max_y + std::max(kOutlineRadius, kShadowOffsetY);
    const auto width = canvas_max_x - canvas_min_x;
    const auto height = canvas_max_y - canvas_min_y;
    if (width <= 0 || height <= 0 || width > kMaxCanvasWidth ||
        height > kMaxCanvasHeight) {
      return false;
    }

    std::fill(fill_mask_.begin(), fill_mask_.end(), 0);
    std::fill(outline_mask_.begin(), outline_mask_.end(), 0);
    if (!rasterize(
          value,
          scale,
          canvas_min_x,
          canvas_min_y,
          width,
          height
        )) {
      return false;
    }
    expand_outline(width, height, kOutlineRadius);

    const auto applied_alpha = renderer->a({0xF, 0xF, 0xF, alpha}).a;
    constexpr std::uint8_t kShadowAlpha = 0xA;
    for (s32 y = 0; y < height; ++y) {
      for (s32 x = 0; x < width; ++x) {
        const auto fill_coverage = mask_value(fill_mask_, x, y);
        const auto outline_coverage = mask_value(outline_mask_, x, y);
        const auto shadow_coverage = mask_value(
          fill_mask_, x - kShadowOffsetX, y - kShadowOffsetY
        );
        const auto fill_alpha = apply_alpha(fill_coverage, applied_alpha);
        const auto outline_alpha =
          apply_alpha(outline_coverage, applied_alpha);
        const auto shadow_alpha = apply_alpha(
          shadow_coverage,
          static_cast<std::uint8_t>(
            applied_alpha * kShadowAlpha / 0xF
          )
        );
        const auto black_alpha = source_over_alpha(
          outline_alpha, shadow_alpha
        );
        const auto output_alpha = source_over_alpha(fill_alpha, black_alpha);
        if (output_alpha == 0) {
          continue;
        }

        // The fill is yellow and both layers underneath are black. Dividing
        // the premultiplied yellow by the combined alpha preserves the dark
        // anti-aliased edge instead of replacing it with a translucent fill.
        const auto yellow_weight = static_cast<std::uint8_t>(
          (static_cast<std::uint16_t>(fill_alpha) * 255 +
           output_alpha / 2) /
          output_alpha
        );
        renderer->setPixel(
          static_cast<s16>(left + canvas_min_x + x),
          static_cast<s16>(baseline_y + canvas_min_y + y),
          {
            color_nibble(0xFF, yellow_weight),
            color_nibble(0xCC, yellow_weight),
            color_nibble(0x33, yellow_weight),
            alpha_nibble(output_alpha),
          }
        );
      }
    }
    return true;
  }

private:
  static constexpr s32 kMaxCanvasWidth = 384;
  static constexpr s32 kMaxCanvasHeight = 96;
  static constexpr s32 kMaxGlyphWidth = 64;
  static constexpr s32 kMaxGlyphHeight = 64;
  static constexpr std::size_t kCanvasPixels =
    static_cast<std::size_t>(kMaxCanvasWidth) * kMaxCanvasHeight;
  static constexpr std::size_t kGlyphPixels =
    static_cast<std::size_t>(kMaxGlyphWidth) * kMaxGlyphHeight;

  bool ensure_font() {
    if (font_initialized_) {
      return true;
    }
    if (font_initialization_attempted_) {
      return false;
    }
    font_initialization_attempted_ = true;
    if (R_FAILED(plGetSharedFontByType(
          &font_data_, PlSharedFontType_Standard
        ))) {
      return false;
    }
    const auto* font_buffer =
      reinterpret_cast<const unsigned char*>(font_data_.address);
    font_initialized_ = stbtt_InitFont(
      &font_,
      font_buffer,
      stbtt_GetFontOffsetForIndex(font_buffer, 0)
    ) != 0;
    return font_initialized_;
  }

  s32 text_advance(const char* value, const float font_size) const {
    const auto scale = stbtt_ScaleForPixelHeight(&font_, font_size);
    s32 pen_x{};
    unsigned char previous{};
    for (const auto* cursor =
           reinterpret_cast<const unsigned char*>(value);
         *cursor != '\0';
         ++cursor) {
      const auto codepoint = *cursor;
      pen_x += static_cast<s32>(
        scale * stbtt_GetCodepointKernAdvance(
                  &font_, previous, codepoint
                )
      );
      int advance{};
      int bearing{};
      stbtt_GetCodepointHMetrics(&font_, codepoint, &advance, &bearing);
      pen_x += static_cast<s32>(advance * scale);
      previous = codepoint;
    }
    return pen_x;
  }

  bool text_bounds(
    const char* value,
    const float scale,
    s32& min_x,
    s32& max_x,
    s32& min_y,
    s32& max_y
  ) const {
    bool has_glyph{};
    s32 pen_x{};
    unsigned char previous{};
    for (const auto* cursor =
           reinterpret_cast<const unsigned char*>(value);
         *cursor != '\0';
         ++cursor) {
      const auto codepoint = *cursor;
      pen_x += static_cast<s32>(
        scale * stbtt_GetCodepointKernAdvance(
                  &font_, previous, codepoint
                )
      );
      int x0{};
      int y0{};
      int x1{};
      int y1{};
      stbtt_GetCodepointBitmapBoxSubpixel(
        &font_, codepoint, scale, scale, 0, 0, &x0, &y0, &x1, &y1
      );
      if (x1 > x0 && y1 > y0) {
        if (!has_glyph) {
          min_x = pen_x + x0;
          max_x = pen_x + x1;
          min_y = y0;
          max_y = y1;
          has_glyph = true;
        } else {
          min_x = std::min(min_x, pen_x + x0);
          max_x = std::max(max_x, pen_x + x1);
          min_y = std::min(min_y, static_cast<s32>(y0));
          max_y = std::max(max_y, static_cast<s32>(y1));
        }
      }
      int advance{};
      int bearing{};
      stbtt_GetCodepointHMetrics(&font_, codepoint, &advance, &bearing);
      pen_x += static_cast<s32>(advance * scale);
      previous = codepoint;
    }
    return has_glyph;
  }

  bool rasterize(
    const char* value,
    const float scale,
    const s32 canvas_min_x,
    const s32 canvas_min_y,
    const s32 width,
    const s32 height
  ) {
    s32 pen_x{};
    unsigned char previous{};
    for (const auto* cursor =
           reinterpret_cast<const unsigned char*>(value);
         *cursor != '\0';
         ++cursor) {
      const auto codepoint = *cursor;
      pen_x += static_cast<s32>(
        scale * stbtt_GetCodepointKernAdvance(
                  &font_, previous, codepoint
                )
      );
      int x0{};
      int y0{};
      int x1{};
      int y1{};
      stbtt_GetCodepointBitmapBoxSubpixel(
        &font_, codepoint, scale, scale, 0, 0, &x0, &y0, &x1, &y1
      );
      const auto glyph_width = x1 - x0;
      const auto glyph_height = y1 - y0;
      if (glyph_width > kMaxGlyphWidth || glyph_height > kMaxGlyphHeight) {
        return false;
      }
      if (glyph_width > 0 && glyph_height > 0) {
        std::fill(glyph_mask_.begin(), glyph_mask_.end(), 0);
        stbtt_MakeCodepointBitmap(
          &font_,
          glyph_mask_.data(),
          glyph_width,
          glyph_height,
          kMaxGlyphWidth,
          scale,
          scale,
          codepoint
        );
        const auto target_x = pen_x + x0 - canvas_min_x;
        const auto target_y = y0 - canvas_min_y;
        for (s32 glyph_y = 0; glyph_y < glyph_height; ++glyph_y) {
          for (s32 glyph_x = 0; glyph_x < glyph_width; ++glyph_x) {
            const auto x = target_x + glyph_x;
            const auto y = target_y + glyph_y;
            if (x < 0 || y < 0 || x >= width || y >= height) {
              continue;
            }
            auto& target = fill_mask_[mask_index(x, y)];
            target = std::max(
              target,
              glyph_mask_[static_cast<std::size_t>(glyph_y) *
                            kMaxGlyphWidth + glyph_x]
            );
          }
        }
      }
      int advance{};
      int bearing{};
      stbtt_GetCodepointHMetrics(&font_, codepoint, &advance, &bearing);
      pen_x += static_cast<s32>(advance * scale);
      previous = codepoint;
    }
    return true;
  }

  void expand_outline(
    const s32 width, const s32 height, const s32 radius
  ) {
    for (s32 y = 0; y < height; ++y) {
      for (s32 x = 0; x < width; ++x) {
        const auto coverage = mask_value(fill_mask_, x, y);
        if (coverage == 0) {
          continue;
        }
        for (s32 offset_y = -radius; offset_y <= radius; ++offset_y) {
          for (s32 offset_x = -radius; offset_x <= radius; ++offset_x) {
            const auto target_x = x + offset_x;
            const auto target_y = y + offset_y;
            if (target_x < 0 || target_y < 0 || target_x >= width ||
                target_y >= height) {
              continue;
            }
            auto& target = outline_mask_[mask_index(target_x, target_y)];
            target = std::max(target, coverage);
          }
        }
      }
    }
  }

  static std::size_t mask_index(const s32 x, const s32 y) {
    return static_cast<std::size_t>(y) * kMaxCanvasWidth + x;
  }

  static std::uint8_t mask_value(
    const std::array<std::uint8_t, kCanvasPixels>& mask,
    const s32 x,
    const s32 y
  ) {
    if (x < 0 || y < 0 || x >= kMaxCanvasWidth || y >= kMaxCanvasHeight) {
      return 0;
    }
    return mask[mask_index(x, y)];
  }

  static std::uint8_t apply_alpha(
    const std::uint8_t coverage, const std::uint8_t alpha
  ) {
    return static_cast<std::uint8_t>(
      (static_cast<std::uint16_t>(coverage) * alpha + 7) / 15
    );
  }

  static std::uint8_t source_over_alpha(
    const std::uint8_t foreground, const std::uint8_t background
  ) {
    return static_cast<std::uint8_t>(
      foreground +
      (static_cast<std::uint16_t>(background) * (255 - foreground) + 127) /
        255
    );
  }

  static std::uint8_t color_nibble(
    const std::uint8_t channel, const std::uint8_t weight
  ) {
    const auto value = static_cast<std::uint8_t>(
      (static_cast<std::uint16_t>(channel) * weight + 127) / 255
    );
    return static_cast<std::uint8_t>((value + 8) / 17);
  }

  static std::uint8_t alpha_nibble(const std::uint8_t alpha) {
    return static_cast<std::uint8_t>((alpha + 8) / 17);
  }

  PlFontData font_data_{};
  stbtt_fontinfo font_{};
  bool font_initialization_attempted_{};
  bool font_initialized_{};
  std::array<std::uint8_t, kCanvasPixels> fill_mask_{};
  std::array<std::uint8_t, kCanvasPixels> outline_mask_{};
  std::array<std::uint8_t, kGlyphPixels> glyph_mask_{};
};

DamageTextRenderer& damage_text_renderer() {
  static DamageTextRenderer renderer;
  return renderer;
}
