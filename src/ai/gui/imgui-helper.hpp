#pragma once

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>

/* ImVec2 has no native operator+. Add for convenience */
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

/* ImVec2 has no native operator+. Add for convenience */
static inline ImVec2 operator+(const ImVec2& lhs, float rhs) {
	return ImVec2(lhs.x + rhs, lhs.y + rhs);
}

/* ImVec2 has no native operator-. Add for convenience */
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}

/* ImVec2 has no native operator-. Add for convenience */
static inline ImVec2 operator-(const ImVec2& lhs, float rhs) {
	return ImVec2(lhs.x - rhs, lhs.y - rhs);
}

/* ImVec2 has no native operator*. Add for convenience */
static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y);
}

/* ImVec2 has no native operator-. Add for convenience */
static inline ImVec2 operator*(const ImVec2& lhs, float rhs) {
	return ImVec2(lhs.x * rhs, lhs.y * rhs);
}

/* ImVec2 has no native operator/. Add for convenience */
static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) {
	return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y);
}

/* ImVec2 has no native operator/. Add for convenience */
static inline ImVec2 operator/(const ImVec2& lhs, float rhs) {
	return ImVec2(lhs.x / rhs, lhs.y / rhs);
}
