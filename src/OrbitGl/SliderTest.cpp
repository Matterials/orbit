// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <math.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "GlCanvas.h"
#include "GlSlider.h"

class MockCanvas : public GlCanvas {
  static constexpr uint32_t kFontSize = 14;

 public:
  MockCanvas() : GlCanvas(kFontSize) {}
  MOCK_METHOD(int, GetWidth, (), (const, override));
  MOCK_METHOD(int, GetHeight, (), (const, override));
};

template <int dim>
void Pick(GlSlider& slider, int start, int other_dim = 0) {
  static_assert(dim >= 0 && dim <= 1);
  if constexpr (dim == 0) {
    slider.OnPick(start, other_dim);
  } else {
    slider.OnPick(other_dim, start);
  }
}

template <int dim>
void Drag(GlSlider& slider, int end, int other_dim = 0) {
  static_assert(dim >= 0 && dim <= 1);
  if constexpr (dim == 0) {
    slider.OnDrag(end, other_dim);
  } else {
    slider.OnDrag(other_dim, end);
  }
}

template <int dim>
void PickDrag(GlSlider& slider, int start, int end = -1, int other_dim = 0) {
  static_assert(dim >= 0 && dim <= 1);
  if (end < 0) {
    end = start;
  }
  Pick<dim>(slider, start, other_dim);
  Drag<dim>(slider, end, other_dim);
}

template <int dim>
void PickDragRelease(GlSlider& slider, int start, int end = -1, int other_dim = 0) {
  static_assert(dim >= 0 && dim <= 1);
  PickDrag<dim>(slider, start, end, other_dim);
  slider.OnRelease();
}

template <typename SliderClass>
std::pair<std::unique_ptr<SliderClass>, std::unique_ptr<MockCanvas>> Setup() {
  std::unique_ptr<MockCanvas> canvas = std::make_unique<MockCanvas>();
  // Simulate a 1000x100 canvas
  EXPECT_CALL(*canvas, GetWidth()).WillRepeatedly(::testing::Return(150));
  EXPECT_CALL(*canvas, GetHeight()).WillRepeatedly(::testing::Return(1050));

  std::unique_ptr<SliderClass> slider = std::make_unique<SliderClass>();
  slider->SetCanvas(canvas.get());
  slider->SetPixelHeight(10);
  slider->SetOrthogonalSliderPixelHeight(50);

  // Set the slider to be 50% of the maximum size, position in the middle
  slider->SetNormalizedPosition(0.5f);
  slider->SetNormalizedLength(0.5f);

  return std::make_pair<std::unique_ptr<SliderClass>, std::unique_ptr<MockCanvas>>(
      std::move(slider), std::move(canvas));
}

const float kEpsilon = 0.01f;

template <typename SliderClass, int dim>
static void TestDragType() {
  auto [slider, canvas] = Setup<SliderClass>();

  const float kPos = 0.5f;
  const float kSize = 0.5f;
  const int kOffset = 2;

  int drag_count = 0;
  float pos = kPos;
  int size_count = 0;
  float size = kSize;

  slider->SetDragCallback([&](float ratio) {
    ++drag_count;
    pos = ratio;
  });
  slider->SetResizeCallback([&](float start, float end) {
    ++size_count;
    size = end - start;
  });

  // Use different scales for x and y to make sure dims are chosen correctly
  int scale = static_cast<int>(pow(10, dim));

  PickDragRelease<dim>(*slider, 50 * scale);
  EXPECT_EQ(drag_count, 1);
  EXPECT_EQ(size_count, 0);
  EXPECT_EQ(pos, kPos);
  EXPECT_EQ(size, kSize);

  PickDragRelease<dim>(*slider, 25 * scale + kOffset);
  if (slider->CanResize()) {
    EXPECT_EQ(drag_count, 2);
    EXPECT_EQ(size_count, 1);
  } else {
    EXPECT_EQ(drag_count, 2);
    EXPECT_EQ(size_count, 0);
  }
  EXPECT_EQ(pos, kPos);
  EXPECT_EQ(size, kSize);

  PickDragRelease<dim>(*slider, 75 * scale - kOffset);
  if (slider->CanResize()) {
    EXPECT_EQ(drag_count, 3);
    EXPECT_EQ(size_count, 2);
  } else {
    EXPECT_EQ(drag_count, 3);
    EXPECT_EQ(size_count, 0);
  }
  EXPECT_EQ(pos, kPos);
  EXPECT_EQ(size, kSize);

  drag_count = 0;
  size_count = 0;

  PickDragRelease<dim>(*slider, kOffset);
  EXPECT_EQ(drag_count, 1);
  EXPECT_EQ(size_count, 0);
  EXPECT_NE(pos, kPos);
  EXPECT_EQ(size, kSize);

  PickDragRelease<dim>(*slider, 100 * scale - kOffset);
  EXPECT_EQ(drag_count, 2);
  EXPECT_EQ(size_count, 0);
  EXPECT_NE(pos, kPos);
  EXPECT_EQ(size, kSize);
}

TEST(Slider, DragType) {
  TestDragType<GlHorizontalSlider, 0>();
  TestDragType<GlVerticalSlider, 1>();
}

template <typename SliderClass, int dim>
static void TestScroll(float slider_length = 0.25) {
  auto [slider, canvas] = Setup<SliderClass>();

  // Use different scales for x and y to make sure dims are chosen correctly
  int scale = static_cast<int>(pow(10, dim));
  float pos;
  const int kOffset = 2;

  slider->SetDragCallback([&](float ratio) { pos = ratio; });
  slider->SetNormalizedLength(slider_length);

  PickDragRelease<dim>(*slider, kOffset);
  EXPECT_LT(pos, 0.5f);
  const float kCurPos = pos;

  PickDragRelease<dim>(*slider, 100 * scale - kOffset);
  EXPECT_GT(pos, kCurPos);
}

TEST(Slider, Scroll) {
  TestScroll<GlHorizontalSlider, 0>();
  TestScroll<GlVerticalSlider, 1>();
}

template <typename SliderClass, int dim>
static void TestDrag(float slider_length = 0.25) {
  auto [slider, canvas] = Setup<SliderClass>();

  // Use different scales for x and y to make sure dims are chosen correctly
  int scale = static_cast<int>(pow(10, dim));
  float pos;

  slider->SetDragCallback([&](float ratio) { pos = ratio; });
  slider->SetNormalizedLength(slider_length);

  Pick<dim>(*slider, 50 * scale);

  // Expect the slider to be dragged all the way to the right
  // (overshoot, first, then go back to exact drag pos)
  Drag<dim>(*slider, 100 * scale);
  EXPECT_NEAR(pos, 1.f, kEpsilon);
  EXPECT_EQ(slider->GetPosRatio(), pos);
  Drag<dim>(*slider, 100 * scale - static_cast<int>(slider->GetPixelLength() / 2));
  EXPECT_NEAR(pos, 1.f, kEpsilon);

  // Drag to middle
  Drag<dim>(*slider, 50 * scale);
  EXPECT_NEAR(pos, 0.5f, kEpsilon);

  // Drag to left
  Drag<dim>(*slider, 0);
  EXPECT_NEAR(pos, 0.f, kEpsilon);

  // Back to middle
  Drag<dim>(*slider, 50 * scale);
  EXPECT_NEAR(pos, 0.5f, kEpsilon);

  // "Invalid" drags sanity check
  Drag<dim>(*slider, 50 * scale, 5000);
  EXPECT_NEAR(pos, 0.5f, kEpsilon);
}

TEST(Slider, Drag) {
  TestDrag<GlHorizontalSlider, 0>();
  TestDrag<GlVerticalSlider, 1>();
}

TEST(Slider, DragBreakTests) {
  TestDrag<GlHorizontalSlider, 0>(0.0001f);
  TestDrag<GlVerticalSlider, 1>(0.0001f);
}

template <typename SliderClass, int dim>
static void TestScaling() {
  auto [slider, canvas] = Setup<SliderClass>();

  if (!slider->CanResize()) {
    return;
  }

  float size = 0.5f;
  float pos = 0.5f;
  const int kOffset = 2;

  // Use different scales for x and y to make sure dims are chosen correctly
  int scale = static_cast<int>(pow(10, dim));

  slider->SetResizeCallback([&](float start, float end) { size = end - start; });
  slider->SetDragCallback([&](float ratio) { pos = ratio; });

  // Pick on the left
  Pick<dim>(*slider, 25 * scale + kOffset);

  // Resize 10% to the left, then all the way
  Drag<dim>(*slider, 15 * scale + kOffset);
  EXPECT_NEAR(size, 0.6f, kEpsilon);
  EXPECT_NEAR(pos, 0.15f / 0.4f, kEpsilon);
  EXPECT_EQ(slider->GetLengthRatio(), size);
  EXPECT_EQ(slider->GetPosRatio(), pos);

  Drag<dim>(*slider, 0);
  EXPECT_NEAR(size, 0.75f, kEpsilon);
  EXPECT_NEAR(pos, 0.f, kEpsilon);

  // Drag back
  Drag<dim>(*slider, 25 * scale + kOffset);
  EXPECT_NEAR(size, 0.5f, kEpsilon);
  EXPECT_NEAR(pos, 0.5f, kEpsilon);
  slider->OnRelease();

  // Pick on the right
  Pick<dim>(*slider, 75 * scale - kOffset);

  // Resize 10% to the right, then all the way
  Drag<dim>(*slider, 85 * scale - kOffset);
  EXPECT_NEAR(size, 0.6f, kEpsilon);
  EXPECT_NEAR(pos, 0.25f / 0.4f, kEpsilon);

  Drag<dim>(*slider, 100 * scale);
  EXPECT_NEAR(size, 0.75f, kEpsilon);
  EXPECT_NEAR(pos, 1.f, kEpsilon);

  // Drag back
  Drag<dim>(*slider, 75 * scale - kOffset);
  EXPECT_NEAR(size, 0.5f, kEpsilon);
  EXPECT_NEAR(pos, 0.25f / 0.5f, kEpsilon);
  slider->OnRelease();
}

TEST(Slider, Scale) {
  TestScaling<GlHorizontalSlider, 0>();
  TestScaling<GlVerticalSlider, 1>();
}

template <typename SliderClass, int dim>
static void TestBreakScaling() {
  auto [slider, canvas] = Setup<SliderClass>();

  if (!slider->CanResize()) {
    return;
  }

  float pos;
  float len;
  const int kOffset = 2;

  // Use different scales for x and y to make sure dims are chosen correctly
  int scale = static_cast<int>(pow(10, dim));

  // Pick on the right, then drag across the end of the slider
  pos = slider->GetPixelPos();
  len = slider->GetPixelLength();
  PickDragRelease<dim>(*slider, 75 * scale - kOffset, 0);
  EXPECT_NEAR(slider->GetPixelPos(), pos, kEpsilon);
  EXPECT_NEAR(slider->GetPixelLength(), slider->GetMinSliderPixelLength(), kEpsilon);

  slider->SetNormalizedPosition(0.5f);
  slider->SetNormalizedLength(0.5f);

  // Pick on the left, then drag across the end of the slider
  PickDragRelease<dim>(*slider, 25 * scale + kOffset, 100 * scale);
  EXPECT_NEAR(slider->GetPixelPos(), pos + len - slider->GetMinSliderPixelLength(), kEpsilon);
  EXPECT_NEAR(slider->GetPixelLength(), slider->GetMinSliderPixelLength(), kEpsilon);
}

TEST(Slider, BreakScale) {
  TestBreakScaling<GlHorizontalSlider, 0>();
  TestBreakScaling<GlVerticalSlider, 1>();
}